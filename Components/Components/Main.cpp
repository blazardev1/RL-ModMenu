#include "Main.hpp"
#include "../Includes.hpp"
#include "../Modules/Includes.hpp"



MainComponent::MainComponent() : Component("Main", "Interface to game interacton") { OnCreate(); }

MainComponent::~MainComponent() { OnDestroy(); }

void MainComponent::OnCreate() {}

void MainComponent::OnDestroy() {}

void MainComponent::Initialize() {

}

void MainComponent::SpawnNotification(const std::string& Title, const std::string& Content, int Duration, UClass* NotificationClass)
{
	Console.Write("Starting simplified notification creation");
	
	if (UNotificationManager_TA* NotificationManager = Instances.GetInstanceOf<UNotificationManager_TA>()) {
		Console.Write("Got NotificationManager");
		
		// Create a notification with built-in title and content
		Console.Write("Creating notification with direct parameters: " + Title);
		
		// Create a notification directly with simple parameters
		UNotification_TA* Notification = NotificationManager->PopUpOnlyNotification(nullptr);
		
		if (Notification) {
			Console.Write("Notification created successfully");
			
			// Set parameters directly using wchar_t arrays
			wchar_t wTitle[256] = {0};
			wchar_t wContent[1024] = {0};
			
			// Convert the strings manually
			for (size_t i = 0; i < Title.length() && i < 255; i++) {
				wTitle[i] = static_cast<wchar_t>(Title[i]);
			}
			
			for (size_t i = 0; i < Content.length() && i < 1023; i++) {
				wContent[i] = static_cast<wchar_t>(Content[i]);
			}
			
			// Create FStrings from the wchar arrays
			Notification->Title = wTitle;
			Notification->Body = wContent;
			Notification->PopUpDuration = Duration;
			
			Console.Write("Notification parameters set");
		} else {
			Console.Write("Failed to create notification");
		}
	} else {
		Console.Write("Failed to get NotificationManager");
	}
}




std::vector<std::function<void()>> MainComponent::GameFunctions;

class MainComponent Main {};