#include "BallPrediction.hpp"
#include "../Manager.hpp"
#include "../Modules/Includes.hpp"
#include "../GUI.hpp"
#include <algorithm>

// Forward declarations
class ABall_TA;
class AWorldInfo;

// Initialize static members
bool BallPrediction::bEnabled = true;
int BallPrediction::iPredictionTime = 3; // seconds
ImVec4 BallPrediction::predictionColor = ImVec4(1.0f, 0.0f, 1.0f, 0.8f); // Magenta with alpha

// Define the global BallPrediction instance
BallPrediction BallPrediction;

BallPrediction::BallPrediction() : Component("Ball Prediction", "Predicts the ball's trajectory") 
{
    // Constructor implementation
}

BallPrediction::~BallPrediction() 
{ 
    OnDestroy(); 
}

void BallPrediction::OnCreate()
{
    // Register the component with the manager
    Manager::RegisterComponent(this);
}

void BallPrediction::OnDestroy()
{
    // Cleanup if needed
}

void BallPrediction::OnRender()
{
    if (!bEnabled) return;

    std::vector<FVector> predictedPositions;
    PredictBallTrajectory(predictedPositions, iPredictionTime * 60); // 60 updates per second
    DrawPrediction(predictedPositions);
}

void BallPrediction::OnRenderMenu()
{
    // This is now handled in GUI.cpp
    // Keeping this as a placeholder in case we need custom menu rendering in the future
}

void BallPrediction::PredictBallTrajectory(std::vector<FVector>& outPositions, int numFrames, float timeStep)
{
    outPositions.clear();
    
    // Get the ball instance
    auto balls = Instances.GetAllInstancesOf<ABall_TA>();
    if (balls.empty()) return;
    
    ABall_TA* ball = balls[0];
    if (!ball) return;
    
    // Get world info for gravity
    AWorldInfo* worldInfo = Instances.IAWorldInfo();
    if (!worldInfo) return;
    
    // Get current ball state
    FVector position = ball->Location;
    FVector velocity = ball->Velocity;
    float gravityZ = worldInfo->GetGravityZ();
    float radius = 100.0f; // Approximate ball radius
    float mass = 1.0f;     // Approximate ball mass
    
    // Air resistance (simplified)
    float dragCoefficient = 0.0005f;
    
    // Simulation
    FVector currentPos = position;
    FVector currentVel = velocity;
    
    for (int i = 0; i < numFrames; ++i)
    {
        // Apply gravity
        currentVel.Z += gravityZ * timeStep;
        
        // Apply air resistance (simplified)
        float speed = currentVel.Size();
        if (speed > 0.1f)
        {
            FVector dragForce = currentVel * (-dragCoefficient * speed * timeStep);
            currentVel = currentVel + dragForce / mass;
        }
        
        // Update position
        currentPos = currentPos + (currentVel * timeStep);
        
        // Check for ground collision (simplified)
        if (currentPos.Z < radius)
        {
            currentPos.Z = radius;
            currentVel.Z = -currentVel.Z * 0.7f; // Bounce coefficient
            
            // Add some friction
            currentVel.X *= 0.9f;
            currentVel.Y *= 0.9f;
        }
        
        outPositions.push_back(currentPos);
    }
}

void BallPrediction::DrawPrediction(const std::vector<FVector>& positions)
{
    if (positions.size() < 2) return;
    
    // Get the viewport and viewport client
    UGameViewportClient* viewportClient = Instances.GetInstanceOf<UGameViewportClient>();
    if (!viewportClient) return;
    
    // Get the background draw list
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;
    
    // Draw the prediction line
    FVector2D screenPos, lastScreenPos;
    bool hasLastPos = false;
    
    for (size_t i = 0; i < positions.size(); ++i)
    {
        if (viewportClient->ProjectWorldToScreen(positions[i], screenPos))
        {
            if (hasLastPos)
            {
                // Fade the line based on distance
                float alpha = 1.0f - (float)i / (float)positions.size();
                ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    predictionColor.x,
                    predictionColor.y,
                    predictionColor.z,
                    predictionColor.w * alpha * 0.8f // Fade out
                ));
                
                // Draw line segment
                drawList->AddLine(
                    ImVec2(lastScreenPos.X, lastScreenPos.Y),
                    ImVec2(screenPos.X, screenPos.Y),
                    color,
                    2.0f);
            }
            lastScreenPos = screenPos;
            hasLastPos = true;
        }
    }
}

// Second implementation of PredictBallTrajectory (remove this duplicate)
void BallPrediction::PredictBallTrajectory(std::vector<FVector>& outPositions, int numFrames, float timeStep)
{
    outPositions.clear();
    
    // Get the ball instance
    auto balls = Instances.GetAllInstancesOf<ABall_TA>();
    if (balls.empty()) return;
    
    ABall_TA* ball = balls[0];
    if (!ball) return;
    
    // Get world info for gravity
    AWorldInfo* worldInfo = Instances.IAWorldInfo();
    if (!worldInfo) return;
    
    // Get current ball state
    FVector position = ball->Location;
    FVector velocity = ball->Velocity;
    FVector angularVelocity = ball->AngularVelocity;
    float radius = ball->GetCollisionRadius();
    float mass = ball->Mass;
    
    // Gravity
    FVector gravity(0, 0, worldInfo->GetGravityZ());
    
    // Air resistance (simplified)
    float dragCoefficient = 0.0005f;
    
    // Simulation
    FVector currentPos = position;
    FVector currentVel = velocity;
    
    for (int i = 0; i < numFrames; ++i)
    {
        // Apply gravity
        currentVel += gravity * timeStep;
        
        // Apply air resistance
        float speed = currentVel.Size();
        if (speed > 0.1f)
        {
            FVector dragForce = -currentVel * (dragCoefficient * speed * speed * timeStep);
            currentVel += dragForce / mass;
        }
        
        // Update position
        currentPos += currentVel * timeStep;
        
        // Check for ground collision (simplified)
        if (currentPos.Z < radius)
        {
            currentPos.Z = radius;
            currentVel.Z = -currentVel.Z * 0.7f; // Bounce coefficient
            
            // Add some friction
            currentVel.X *= 0.9f;
            currentVel.Y *= 0.9f;
        }
        
        outPositions.push_back(currentPos);
    }
}
