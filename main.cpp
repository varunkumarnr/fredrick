#include <U8g2lib.h>
#include <Wire.h>

// Create a U8G2 object for your 128x64 OLED, using I2C pins (GPIO 4, 5)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/4, /* data=*/5);

unsigned long lastBlinkTime = 0;
bool isBlinking = false;
int blinkDuration = 150; 
int blinkInterval = 4000; 

// Track current expression
enum EyeExpression {
  HAPPY,
  SAD,
  NEUTRAL,
  WINK,
  ANGRY,
  SURPRISED,
  CRYING, 
  SLEEPY,
  SLEEPING
};

// Idle animation states
int eyeOffsetX = 0;  // For eye movement
int eyeOffsetY = 0;
unsigned long lastEyeMoveTime = 0;
int eyeMoveInterval = 1500;  // Change eye position every 1.5 seconds

// Mouth animation
int mouthOffsetX = 0;  // Mouth position offsets
int mouthOffsetY = 0;
unsigned long lastMouthMoveTime = 0;
int mouthMoveInterval = 2000;  // Change mouth position every 2 seconds

// Tear animation variables
unsigned long lastTearUpdateTime = 0;
int tearUpdateInterval = 150;
int tearFrame = 0;  // For alternating tear animation
int tearCount = 0;  // To track tear animation cycles

EyeExpression currentExpression = HAPPY;
unsigned long lastExpressionChange = 0;
int expressionDuration = 5000; // Change expression every 5 seconds

float sleepBubblePhase = 0.0f;
float bubbleLifecycles[3] = {0.0f, 2.1f, 4.2f}; // Start at different phases
bool bubbleActive[3] = {true, true, true};
const float MAX_LIFECYCLE = 6.28f;

void setup() {
  u8g2.begin();
  u8g2.setDrawColor(1); // White
  u8g2.setFont(u8g2_font_helvB12_tr);
  
  // Initialize random seed
  randomSeed(analogRead(0));
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to blink
  if (!isBlinking && currentMillis - lastBlinkTime >= blinkInterval) {
    isBlinking = true;
    lastBlinkTime = currentMillis;
  } 
  else if (isBlinking && currentMillis - lastBlinkTime >= blinkDuration) {
    isBlinking = false;
    lastBlinkTime = currentMillis;
    // Randomize next blink interval slightly (3-5 seconds)
    blinkInterval = random(3000, 5000);
  }

  // Update tear animation
  if (currentExpression == CRYING && currentMillis - lastTearUpdateTime >= tearUpdateInterval) {
    tearFrame = !tearFrame;  // Toggle between 0 and 1
    lastTearUpdateTime = currentMillis;
    tearCount++;
    
    // After 20 tear frames (about 3 seconds), reset tears position
    if (tearCount >= 20) {
      tearCount = 0;
    }
  }

  // Update random eye movements when idle
  if (currentMillis - lastEyeMoveTime >= eyeMoveInterval) {
    // Make eyes look in a wilder random direction
    eyeOffsetX = random(-8, 9);  // -8 to +8 pixels
    eyeOffsetY = random(-5, 6);  // -5 to +5 pixels

    // Move mouth exactly the same as eyes
    mouthOffsetX = eyeOffsetX;
    mouthOffsetY = eyeOffsetY;

    lastEyeMoveTime = currentMillis;
    lastMouthMoveTime = currentMillis;

    // Randomize next movement interval (same for both)
    eyeMoveInterval = random(500, 2500);
    mouthMoveInterval = eyeMoveInterval; // Sync intervals exactly
  }
  
  // Change expression periodically
  if (currentMillis - lastExpressionChange >= expressionDuration) {
    // Cycle through expressions including CRYING
    switch (currentExpression) {
      case HAPPY:
        currentExpression = SAD;
        break;
      case SAD:
        currentExpression = NEUTRAL;
        break;
      case NEUTRAL:
        currentExpression = ANGRY;
        break;
      case ANGRY:
        currentExpression = SURPRISED;
        break;
      case SURPRISED:
        currentExpression = SLEEPY;  // Added CRYING to cycle
        break;
      case SLEEPY: 
        currentExpression = SLEEPING; 
        break;
      case SLEEPING: 
        currentExpression = CRYING; 
        break;
      case CRYING:
        currentExpression = HAPPY;
        break;
    }
    lastExpressionChange = currentMillis;
    // Randomize next expression duration (4-7 seconds)
    expressionDuration = random(4000, 7000);
  }

  u8g2.clearBuffer();
  
  if (isBlinking && currentExpression != SLEEPING) {
    drawBlinkingEyes();
  } else {
    // Draw the current expression
    switch (currentExpression) {
      case HAPPY:
        drawHappyEyes();
        break;
      case SAD:
        drawSadEyes();
        break;
      case NEUTRAL:
        drawNeutralEyes();
        break;
      case ANGRY:
        drawAngryEyes();
        break;
      case SURPRISED:
        drawSurprisedEyes();
        break;
      case CRYING:
        drawCryingEyes(tearFrame);
        break;
      case SLEEPY: 
        drawSleepyEyes();
        break;
      case SLEEPING:
        updateSleepBubblePhase();  
        drawSleepingEyes();
        break;
    }
  }


  u8g2.sendBuffer();

  delay(30); // small delay to smooth out updates
}

// Helper function for anti-aliased line drawing
void drawSmoothLine(int x0, int y0, int x1, int y1, float thickness = 1.0) {
  // Bresenham's algorithm with anti-aliasing
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  float intensity;
  int e2;
  
  while (true) {
    // Draw main pixel at full intensity
    u8g2.drawPixel(x0, y0);
    
    // Draw surrounding pixels with varying intensity for anti-aliasing
    if (thickness > 1.0) {
      // Vertical thickness
      if (dx > dy) {
        u8g2.drawPixel(x0, y0 + 1);
        u8g2.drawPixel(x0, y0 - 1);
      } 
      // Horizontal thickness
      else {
        u8g2.drawPixel(x0 + 1, y0);
        u8g2.drawPixel(x0 - 1, y0);
      }
    }
    
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

// Helper function to draw anti-aliased filled circle
void drawSmoothFilledCircle(int x0, int y0, int radius) {
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x*x + y*y <= radius*radius) {
        u8g2.drawPixel(x0 + x, y0 + y);
      } 
      // Anti-aliasing for edge pixels
      else if (x*x + y*y <= (radius+1)*(radius+1)) {
        // Check if it's near the edge for anti-aliasing
        float distance = sqrtf(x*x + y*y);
        if (distance < radius + 1.0f && distance > radius - 0.5f) {
          // Only fill edge pixels that would smooth curve
          u8g2.drawPixel(x0 + x, y0 + y);
        }
      }
    }
  }
}

// Draw smooth oval with anti-aliasing
void drawSmoothOval(int centerX, int centerY, int width, int height) {
  for (int y = -height/2 - 1; y <= height/2 + 1; y++) {
    for (int x = -width/2 - 1; x <= width/2 + 1; x++) {
      float normalizedX = (float)x / (width/2.0f);
      float normalizedY = (float)y / (height/2.0f);
      float distance = normalizedX*normalizedX + normalizedY*normalizedY;
      
      if (distance <= 1.0f) {
        // Inside the oval - full intensity pixel
        u8g2.drawPixel(centerX + x, centerY + y);
      }
      // Edge pixel - anti-aliasing (slightly outside boundary)
      else if (distance < 1.15f) {
        // Draw edge pixels to smooth the curve
        u8g2.drawPixel(centerX + x, centerY + y);
      }
    }
  }
}

void drawHappyEyes() {
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 24 + eyeOffsetY;
  const int eyeWidth = 14;
  const int eyeHeight = 20;
  const int mouthY = 52 + mouthOffsetY;

  // Draw smooth ovals for eyes
  drawSmoothOval(leftEyeX, eyeY, eyeWidth, eyeHeight);
  drawSmoothOval(rightEyeX, eyeY, eyeWidth, eyeHeight);
  
  // Draw smile with subtle movement and smoothing
  for (int x = -10; x <= 10; x++) { 
    float xf = (float)x / 10.0f;  // Normalize x
    float y = -(xf * xf) * 6.0f;  // Smoother parabola
    
    // Main curve with anti-aliasing
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y);
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y + 1); // Thicker line
    
    // Anti-aliasing for smoother edges at the ends
    if (abs(x) >= 8) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y - 1);
    }
  }
}

void drawSadEyes() {
  // Eye parameters
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeCenterY = 28 + eyeOffsetY; // Move a bit down for better proportions
  const int eyeWidth = 16;
  const int eyeHeight = 16;
  const int mouthY = 54 + mouthOffsetY;

  // Helper lambda to draw smooth sad eyes
  auto drawSmoothSadEye = [](int centerX, int centerY, bool slantLeft) {
    for (int x = -8; x <= 8; x++) {
      float xf = (float)x / 8.0f; // normalize x from -1 to 1
      float curve = (1.0f - xf * xf) * 6.0f; // eye shape curve
      int slant = (int)(xf * 4.0f); // slant: stronger, proportional to x
      if (slantLeft) slant = -slant;
      int yBase = centerY + slant;
      
      // Draw main curve with anti-aliasing
      for (int y = 0; y < (int)curve; y++) {
        u8g2.drawPixel(centerX + x, yBase + y);
      }
      
      // Anti-aliasing for edges
      if (abs(x) >= 6) {
        u8g2.drawPixel(centerX + x, yBase + (int)curve);
      }
    }
  };

  // Draw both eyes with smooth edges
  drawSmoothSadEye(leftEyeX, eyeCenterY, true);  // Left eye slants downward
  drawSmoothSadEye(rightEyeX, eyeCenterY, false); // Right eye slants upward

  // Draw sad mouth - soft arc with anti-aliasing
  for (int x = -12; x <= 12; x++) { 
    float xf = (float)x / 12.0f; // normalize
    float y = 4.0f * (xf*xf); // upward curve (sad mouth)
    
    // Draw main curve with anti-aliasing
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y);
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y + 1); // slight thickness
    
    // Add anti-aliasing at the ends
    if (abs(x) >= 10) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y - 1);
    }
  }
}

void drawNeutralEyes() {
  // Improved eye parameters with more spacing
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 24 + eyeOffsetY;
  const int eyeWidth = 14;
  const int eyeHeight = 20;
  const int mouthY = 52 + mouthOffsetY;

  // Draw left eye (1/4th closed) with smoother edges
  for (int x = leftEyeX - eyeWidth/2 - 1; x <= leftEyeX + eyeWidth/2 + 1; x++) {
    for (int y = eyeY - eyeHeight/2 + eyeHeight/4 - 1; y <= eyeY + eyeHeight/2 + 1; y++) { 
      float dx = (float)(x - leftEyeX) / (eyeWidth / 2.0f);
      float dy = (float)(y - eyeY) / (eyeHeight / 2.0f);
      float distSquared = dx*dx + dy*dy;
      
      if (distSquared <= 1.0f && y >= eyeY - eyeHeight/2 + eyeHeight/4) { 
        u8g2.drawPixel(x, y);
      }
      // Anti-aliasing for edges
      else if (distSquared <= 1.2f && distSquared > 0.9f && y >= eyeY - eyeHeight/2 + eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
    }
  }

  // Draw right eye (1/4th closed) with smoother edges
  for (int x = rightEyeX - eyeWidth/2 - 1; x <= rightEyeX + eyeWidth/2 + 1; x++) {
    for (int y = eyeY - eyeHeight/2 + eyeHeight/4 - 1; y <= eyeY + eyeHeight/2 + 1; y++) { 
      float dx = (float)(x - rightEyeX) / (eyeWidth / 2.0f);
      float dy = (float)(y - eyeY) / (eyeHeight / 2.0f);
      float distSquared = dx*dx + dy*dy;
      
      if (distSquared <= 1.0f && y >= eyeY - eyeHeight/2 + eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
      // Anti-aliasing for edges
      else if (distSquared <= 1.2f && distSquared > 0.9f && y >= eyeY - eyeHeight/2 + eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
    }
  }

  // Draw neutral mouth - flat line with offset and anti-aliasing
  for (int x = -10; x <= 10; x++) { 
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY);
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + 1); // slight thickness
    
    // Anti-aliasing at the ends
    if (abs(x) >= 8) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY - 1);
    }
  }
}

void drawSleepyEyes() {
  // Eye parameters with slight adjustments for sleepy look
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 24 + eyeOffsetY;
  const int eyeWidth = 14;
  const int eyeHeight = 20;
  const int mouthY = 54 + mouthOffsetY;

  // Draw left eye (3/4 closed) with smoother edges
  for (int x = leftEyeX - eyeWidth/2 - 1; x <= leftEyeX + eyeWidth/2 + 1; x++) {
    for (int y = eyeY - eyeHeight/2 + 3*eyeHeight/4 - 1; y <= eyeY + eyeHeight/2 + 1; y++) { 
      float dx = (float)(x - leftEyeX) / (eyeWidth / 2.0f);
      float dy = (float)(y - eyeY) / (eyeHeight / 2.0f);
      float distSquared = dx*dx + dy*dy;
      
      if (distSquared <= 1.0f && y >= eyeY - eyeHeight/2 + 3*eyeHeight/4) { 
        u8g2.drawPixel(x, y);
      }
      // Anti-aliasing for edges
      else if (distSquared <= 1.2f && distSquared > 0.9f && y >= eyeY - eyeHeight/2 + 3*eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
    }
  }

  // Draw right eye (3/4 closed) with smoother edges
  for (int x = rightEyeX - eyeWidth/2 - 1; x <= rightEyeX + eyeWidth/2 + 1; x++) {
    for (int y = eyeY - eyeHeight/2 + 3*eyeHeight/4 - 1; y <= eyeY + eyeHeight/2 + 1; y++) { 
      float dx = (float)(x - rightEyeX) / (eyeWidth / 2.0f);
      float dy = (float)(y - eyeY) / (eyeHeight / 2.0f);
      float distSquared = dx*dx + dy*dy;
      
      if (distSquared <= 1.0f && y >= eyeY - eyeHeight/2 + 3*eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
      // Anti-aliasing for edges
      else if (distSquared <= 1.2f && distSquared > 0.9f && y >= eyeY - eyeHeight/2 + 3*eyeHeight/4) {
        u8g2.drawPixel(x, y);
      }
    }
  }

  // Draw slightly open mouth (small horizontal line)
  for (int x = -7; x <= 7; x++) { 
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY);
    // Add very slight curve downward at the ends to show relaxation
    if (abs(x) >= 5) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + 1);
    }
  }
}

void drawSleepBubble(int centerX, int centerY) {
  // Parameters for bubble sequence - REDUCED SIZE
  const int numBubbles = 3;
  int baseBubbleSizes[numBubbles] = {2, 3, 5}; // Smaller bubbles
  // Reposition bubbles more to the side and higher
  int xOffsets[numBubbles] = {2, 8, 16};
  int yOffsets[numBubbles] = {-2, -5, -10}; // Higher position
  
  // Draw Z character (smaller and repositioned)
  int zX = centerX + xOffsets[2] + 4;
  int zY = centerY + yOffsets[2] - 4;
  
  // Apply subtle movement to Z
  float zOffset = sin(sleepBubblePhase * 0.5) * 0.8;
  zY += zOffset;
  
  // Draw top horizontal of Z (smaller)
  for (int i = 0; i < 4; i++) {
    u8g2.drawPixel(zX + i, zY);
  }
  
  // Draw diagonal of Z (smaller)
  for (int i = 0; i < 4; i++) {
    u8g2.drawPixel(zX + 3 - i, zY + i);
  }
  
  // Draw bottom horizontal of Z (smaller)
  for (int i = 0; i < 4; i++) {
    u8g2.drawPixel(zX + i, zY + 3);
  }
  
  // Draw bubbles with appearance/disappearance cycle
  for (int b = 0; b < numBubbles; b++) {
    // Calculate lifecycle phase for this bubble (0.0 - 6.28)
    updateBubbleLifecycle(b);
    
    // Only draw bubble if it's active
    if (bubbleActive[b]) {
      // Calculate size based on lifecycle
      // Start small -> grow -> stay -> shrink -> disappear
      float lifeCycleProgress = bubbleLifecycles[b] / MAX_LIFECYCLE;
      
      // Size curve: start at 0, peak at 50%, end at 0
      float sizeMultiplier;
      if (lifeCycleProgress < 0.2f) {
        // Growing phase (0% - 20% of lifecycle)
        sizeMultiplier = lifeCycleProgress * 5.0f;
      } else if (lifeCycleProgress > 0.8f) {
        // Shrinking phase (80% - 100% of lifecycle)
        sizeMultiplier = (1.0f - lifeCycleProgress) * 5.0f;
      } else {
        // Stable phase with slight breathing (20% - 80% of lifecycle)
        sizeMultiplier = 1.0f + sin((lifeCycleProgress - 0.2f) * 10.0f) * 0.1f;
      }
      
      // Calculate final radius with size curve applied
      int radius = baseBubbleSizes[b] * sizeMultiplier;
      
      // Skip drawing if bubble is too small
      if (radius < 1) continue;
      
      // Calculate position with vertical movement
      int bubbleX = centerX + xOffsets[b];
      // Add rising effect (bubbles rise more as they age)
      int bubbleY = centerY + yOffsets[b] - (lifeCycleProgress * 2.5f);
      
      // Draw bubble with anti-aliasing
      for (int x = -radius-1; x <= radius+1; x++) {
        for (int y = -radius-1; y <= radius+1; y++) {
          float distance = sqrt(x*x + y*y);
          
          if (distance <= radius) {
            u8g2.drawPixel(bubbleX + x, bubbleY + y);
          }
          // Anti-aliasing for edges
          else if (distance <= radius + 1.0f && distance > radius) {
            // Only draw some pixels for anti-aliasing effect
            if ((x + y) % 2 == 0) {
              u8g2.drawPixel(bubbleX + x, bubbleY + y);
            }
          }
        }
      }
    }
  }
}

// Update individual bubble lifecycle
void updateBubbleLifecycle(int bubbleIndex) {
  // Progress the lifecycle
  bubbleLifecycles[bubbleIndex] += 0.1f;
  
  // Reset lifecycle when complete
  if (bubbleLifecycles[bubbleIndex] >= MAX_LIFECYCLE) {
    bubbleLifecycles[bubbleIndex] = 0.0f;
    
    // Random chance to activate/deactivate bubble
    bubbleActive[bubbleIndex] = (random(100) < 80); // 80% chance of being active
  }
}

void updateSleepBubblePhase() {
  // Update main phase counter (for Z movement)
  sleepBubblePhase += 0.05f;
  if (sleepBubblePhase >= TWO_PI) {
    sleepBubblePhase -= TWO_PI;
  }
  
  // Make sure at least one bubble is always active
  bool anyActive = false;
  for (int i = 0; i < 3; i++) {
    if (bubbleActive[i]) {
      anyActive = true;
      break;
    }
  }
  
  // Force one random bubble to be active if none are
  if (!anyActive) {
    bubbleActive[random(3)] = true;
  }
}

void drawSleepingEyes() {
  // Eye parameters
  const int leftEyeX = 35 ;
  const int rightEyeX = 93 ;
  const int eyeY = 24 ;
  const int eyeWidth = 14;
  const int eyeHeight = 4; // Much flatter for closed eyes
  const int mouthY = 54 ;
  
  // Draw left closed eye (horizontal line with slight curve)
  for (int x = -eyeWidth/2; x <= eyeWidth/2; x++) {
    float xNorm = (float)x / (eyeWidth/2);
    float curve = 2.0f * (1.0f - xNorm * xNorm); // Subtle curve
    
    u8g2.drawPixel(leftEyeX + x, eyeY - (int)curve);
    u8g2.drawPixel(leftEyeX + x, eyeY - (int)curve + 1); // Thickness
    
    // Anti-aliasing at the edges
    if (abs(x) >= eyeWidth/2 - 2) {
      u8g2.drawPixel(leftEyeX + x, eyeY - (int)curve - 1);
    }
  }
  
  // Draw right closed eye (horizontal line with slight curve)
  for (int x = -eyeWidth/2; x <= eyeWidth/2; x++) {
    float xNorm = (float)x / (eyeWidth/2);
    float curve = 2.0f * (1.0f - xNorm * xNorm); // Subtle curve
    
    u8g2.drawPixel(rightEyeX + x, eyeY - (int)curve);
    u8g2.drawPixel(rightEyeX + x, eyeY - (int)curve + 1); // Thickness
    
    // Anti-aliasing at the edges
    if (abs(x) >= eyeWidth/2 - 2) {
      u8g2.drawPixel(rightEyeX + x, eyeY - (int)curve - 1);
    }
  }

  // Add subtle breathing movement to the mouth
  float mouthOffset = sin(sleepBubblePhase) * 0.5;
  int adjustedMouthY = mouthY + mouthOffset;
  
  // Draw slightly open relaxed mouth with subtle movement
  for (int x = -5; x <= 5; x++) { 
    u8g2.drawPixel(64 + x + mouthOffsetX, adjustedMouthY);
  }
  
  // Draw sleep bubble near nose (animated with disappearing/reappearing effect)
  // Moved bubble higher and more to the right to avoid covering eyes
  drawSleepBubble(68, eyeY + 10); // Repositioned to better avoid covering eyes
}



void drawWinkEyes() {
  // Improved eye parameters with more spacing
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 28 + eyeOffsetY;
  const int eyeWidth = 20;
  const int eyeHeight = 12;
  const int mouthY = 52 + mouthOffsetY;
  
  // Left eye winks (closed) with smooth edges
  drawSmoothLine(leftEyeX - 7, eyeY, leftEyeX + 7, eyeY, 2.0);
  
  // Right eye - normal eye (slight curve)
  for (int x = -7; x <= 7; x++) {
    float xf = (float)x / 7.0f;
    float y = -abs(xf) * 3.0f - 1.0f; // Slight upward curve
    
    // Main curve with anti-aliasing
    u8g2.drawPixel(rightEyeX + x, eyeY + y);
    u8g2.drawPixel(rightEyeX + x, eyeY + y + 1); // Make thicker
    
    // Anti-aliasing at the ends
    if (abs(x) >= 5) {
      u8g2.drawPixel(rightEyeX + x, eyeY + y - 1);
    }
  }
  
  // Draw smile mouth with smoother edges
  for (int x = -15; x <= 15; x++) {
    float xf = (float)x / 15.0f;
    float y = -abs(xf/2) * 6.0f + 3.0f; // Upward curve
    
    // Main curve
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y);
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y + 1); // Make thicker
    
    // Anti-aliasing at the ends
    if (abs(x) >= 12) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y - 1);
    }
  }
}

void drawAngryEyes() {
  // Eye parameters
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeTopY = 20 + eyeOffsetY;
  const int eyeWidth = 20;
  const int eyeHeight = 12; // Narrower eyes for anger
  const int mouthY = 52 + mouthOffsetY;

  // Draw Left Eye (outer higher, inner lower) - angry with smooth edges
  for (int x = -eyeWidth/2; x <= eyeWidth/2; x++) {
    float xf = (float)x / (eyeWidth/2.0f);
    float slantOffset = xf * 4.0f; // outer side higher (positive slope)
    float curveHeight = (xf * xf) * 4.0f; // Smoother curve
    
    for (int y = 0; y <= eyeHeight - curveHeight; y++) {
      u8g2.drawPixel(leftEyeX + x, eyeTopY + y + slantOffset);
    }
    
    // Anti-aliasing for edges
    if (abs(x) >= eyeWidth/2 - 2) {
      u8g2.drawPixel(leftEyeX + x, eyeTopY + eyeHeight - curveHeight + 1 + slantOffset);
    }
  }

  // Draw Right Eye (outer higher, inner lower) - angry with smooth edges
  for (int x = -eyeWidth/2; x <= eyeWidth/2; x++) {
    float xf = (float)x / (eyeWidth/2.0f);
    float slantOffset = -xf * 4.0f; // outer side higher (negative slope)
    float curveHeight = (xf * xf) * 4.0f;
    
    for (int y = 0; y <= eyeHeight - curveHeight; y++) {
      u8g2.drawPixel(rightEyeX + x, eyeTopY + y + slantOffset);
    }
    
    // Anti-aliasing for edges
    if (abs(x) >= eyeWidth/2 - 2) {
      u8g2.drawPixel(rightEyeX + x, eyeTopY + eyeHeight - curveHeight + 1 + slantOffset);
    }
  }

  // Angry mouth - flat or slightly downward with offset
  const int mouthWidth = 24;
  const int mouthHeight = 6;
  const int mouthCenterX = 64 + mouthOffsetX;
  const int mouthCenterY = mouthY;

  // Draw mouth rectangle (outline) with smoother corners
  for (int x = -mouthWidth/2; x <= mouthWidth/2; x++) {
    u8g2.drawPixel(mouthCenterX + x, mouthCenterY - mouthHeight/2); // top border
    u8g2.drawPixel(mouthCenterX + x, mouthCenterY + mouthHeight/2); // bottom border
    
    // Smooth corners
    if (abs(x) >= mouthWidth/2 - 2) {
      u8g2.drawPixel(mouthCenterX + x, mouthCenterY - mouthHeight/2 - 1);
      u8g2.drawPixel(mouthCenterX + x, mouthCenterY + mouthHeight/2 + 1);
    }
  }
  
  for (int y = -mouthHeight/2; y <= mouthHeight/2; y++) {
    u8g2.drawPixel(mouthCenterX - mouthWidth/2, mouthCenterY + y); // left border
    u8g2.drawPixel(mouthCenterX + mouthWidth/2, mouthCenterY + y); // right border
    
    // Smooth corners
    if (abs(y) >= mouthHeight/2 - 1) {
      u8g2.drawPixel(mouthCenterX - mouthWidth/2 - 1, mouthCenterY + y);
      u8g2.drawPixel(mouthCenterX + mouthWidth/2 + 1, mouthCenterY + y);
    }
  }

  // Draw vertical lines inside mouth for gritted teeth with smoother edges
  for (int x = -mouthWidth/2 + 4; x < mouthWidth/2; x += 5) { 
    for (int y = -mouthHeight/2 + 1; y < mouthHeight/2; y++) {
      u8g2.drawPixel(mouthCenterX + x, mouthCenterY + y);
    }
  }
}

void drawSurprisedEyes() {
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 24 + eyeOffsetY;
  const int eyeWidth = 14;
  const int eyeHeight = 20;
  const int mouthY = eyeY + 30 + mouthOffsetY; 
  const int mouthWidth = 20;
  const int mouthHeight = 10; 

  // Draw smooth ovals for eyes
  drawSmoothOval(leftEyeX, eyeY, eyeWidth, eyeHeight);
  drawSmoothOval(rightEyeX, eyeY, eyeWidth, eyeHeight);

  // Draw a filled "reverse U" mouth with anti-aliasing
  for (int x = 0; x <= mouthWidth / 2; x++) {
    float normX = (float)x / (mouthWidth / 2.0f); 
    float yLimit = sqrtf(1.0f - normX * normX) * mouthHeight; // Curve for upper part
    
    // Draw upper curved part with anti-aliasing
    for (int y = 0; y <= yLimit; y++) {
      u8g2.drawPixel(64 + mouthOffsetX + x, mouthY - y); // Upper curve right
      u8g2.drawPixel(64 + mouthOffsetX - x, mouthY - y); // Upper curve left
    }
    
    // Extra pixels for smoothness at the curve's edge
    if (x > 0 && x < mouthWidth/2 - 1) {
      float edgeY = sqrtf(1.0f - normX * normX) * mouthHeight;
      u8g2.drawPixel(64 + mouthOffsetX + x, mouthY - edgeY - 1);
      u8g2.drawPixel(64 + mouthOffsetX - x, mouthY - edgeY - 1);
    }

    // Draw flat bottom
    u8g2.drawPixel(64 + mouthOffsetX + x, mouthY); // Bottom flat line
    u8g2.drawPixel(64 + mouthOffsetX - x, mouthY);
  }
}

void drawCryingEyes(int tearFrame) {
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 24 + eyeOffsetY;
  const int eyeWidth = 14;
  const int eyeHeight = 20;
  const int tearLength = 18; // Length of falling tears
  const int mouthY = 52 + mouthOffsetY;

  // Draw sad eyes (similar to drawSadEyes but with tears)
  // Draw smooth ovals for eyes
  drawSmoothOval(leftEyeX, eyeY, eyeWidth, eyeHeight);
  drawSmoothOval(rightEyeX, eyeY, eyeWidth, eyeHeight);
  
  // Draw sad mouth - same as in drawSadEyes() but with anti-aliasing
  for (int x = -12; x <= 12; x++) { 
    float xf = (float)x / 12.0f; // normalize
    float y = 4.0f * (xf*xf); // upward curve (sad mouth)
    
    // Draw main curve with anti-aliasing
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y);
    u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y + 1); // slight thickness
    
    // Add anti-aliasing at the ends
    if (abs(x) >= 10) {
      u8g2.drawPixel(64 + x + mouthOffsetX, mouthY + y - 1);
    }
  }

  // Draw zigzag tears with animation
  // Function to draw a tear drop with zigzag pattern
  auto drawTear = [&](int centerX, int startY, int frame, int offset) {
    // Calculate tear position based on animation progress
    int tearProgress = (tearCount + offset) % tearLength;
    
    // Draw the entire tear track
    for (int i = 0; i < tearLength; i++) {
      // Only draw the tear if it's in the current position (for falling effect)
      if (i <= tearProgress) {
        // Create zigzag pattern
        int zigzag = (i % 3 == 0) ? ((frame == 1) ? 1 : -1) : 0;
        
        // Draw tear droplet (thicker at the bottom)
        u8g2.drawPixel(centerX + zigzag, startY + i);
        
        // Make tear wider at bottom for a droplet effect
        if (i >= tearLength - 4) {
          u8g2.drawPixel(centerX + zigzag - 1, startY + i);
          u8g2.drawPixel(centerX + zigzag + 1, startY + i);
        }
        
        // Extra thickness in middle of tear track for visibility
        if (i >= 2 && i < tearLength - 4) {
          // Alternating sides for zigzag effect
          if (i % 2 == 0) {
            u8g2.drawPixel(centerX + zigzag + 1, startY + i);
          } else {
            u8g2.drawPixel(centerX + zigzag - 1, startY + i);
          }
        }
      }
    }
  };

  // Draw tears from both eyes
  drawTear(leftEyeX, eyeY + eyeHeight/2, tearFrame, 0);
  drawTear(rightEyeX, eyeY + eyeHeight/2, !tearFrame, 2); // Slight offset for right eye tear
}

void drawBlinkingEyes() {
  // Improved eye parameters with more spacing
  const int leftEyeX = 35 + eyeOffsetX;
  const int rightEyeX = 93 + eyeOffsetX;
  const int eyeY = 28 + eyeOffsetY;
  
  // Draw closed eyes - just horizontal lines with anti-aliasing
  drawSmoothLine(leftEyeX - 7, eyeY, leftEyeX + 7, eyeY, 2.0);
  drawSmoothLine(rightEyeX - 7, eyeY, rightEyeX + 7, eyeY, 2.0);
}

// Helper function for drawing thick circles with anti-aliasing
void drawSmoothThickCircle(int x0, int y0, int radius, float thickness = 1.0) {
  // Draw outer and inner circles for thickness
  for (int angle = 0; angle < 360; angle++) {
    float radians = angle * PI / 180.0;
    float x = cos(radians) * radius;
    float y = sin(radians) * radius;
    
    // Draw main circle
    u8g2.drawPixel(x0 + x, y0 + y);
	 if (thickness > 1.0) {
      // Inner thickness
      float innerRadius = radius - 0.7;
      x = cos(radians) * innerRadius;
      y = sin(radians) * innerRadius;
      u8g2.drawPixel(x0 + x, y0 + y);
      
      // Outer thickness
      float outerRadius = radius + 0.7;
      x = cos(radians) * outerRadius;
      y = sin(radians) * outerRadius;
      u8g2.drawPixel(x0 + x, y0 + y);
    }
  }
}

// Helper function for drawing thick lines with anti-aliasing
void drawThickLine(int x0, int y0, int x1, int y1) {
  // Draw main line
  u8g2.drawLine(x0, y0, x1, y1);
  
  // Make the line thicker by drawing additional lines with anti-aliasing
  if (x0 == x1) {
    // Vertical line, thicken horizontally
    u8g2.drawLine(x0 - 1, y0, x1 - 1, y1);
    u8g2.drawLine(x0 + 1, y0, x1 + 1, y1);
  } else if (y0 == y1) {
    // Horizontal line, thicken vertically
    u8g2.drawLine(x0, y0 - 1, x1, y1 - 1);
    u8g2.drawLine(x0, y0 + 1, x1, y1 + 1);
  } else {
    // Diagonal line, thicken in both directions
    u8g2.drawLine(x0 + 1, y0, x1 + 1, y1);
    u8g2.drawLine(x0, y0 + 1, x1, y1 + 1);
    
    // Add anti-aliasing at endpoints
    u8g2.drawPixel(x0 - 1, y0);
    u8g2.drawPixel(x1 + 1, y1);
  }
}
