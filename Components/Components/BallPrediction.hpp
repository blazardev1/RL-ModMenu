#pragma once
#include "../Component.hpp"
#include "../Includes.hpp"
#include "../ImGui/imgui.h"
#include <vector>

// Forward declarations
class FVector;

class BallPrediction : public Component
{
public:
    BallPrediction();
    virtual ~BallPrediction();

    virtual void OnCreate();
    virtual void OnDestroy();
    
    // Custom render methods (not part of base Component class)
    void OnRender();
    void OnRenderMenu();

    static void PredictBallTrajectory(std::vector<FVector>& outPositions, int numFrames = 120, float timeStep = 0.0167f);
    static void DrawPrediction(const std::vector<FVector>& positions);

    // Public members to be accessed by GUI
    static bool bEnabled;
    static int iPredictionTime;
    static ImVec4 predictionColor;  // Using ImVec4 instead of ImColor for consistency with ImGui
};

// Forward declare the global instance
extern BallPrediction BallPrediction;
