$filePath = "f:\RL mod cheats\Mod Template\Components\Components\GUI.cpp"
$content = [System.IO.File]::ReadAllText($filePath)

# The target content to find and replace
$target = @'
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Only works in Training mode");
                }

                ImGui::EndTabItem();
'@

# The replacement content
$replacement = @'
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Only works in Training mode");
                }

                // Alt+F4 Toggle
                static bool enableAltF4 = false;
                static bool altPressed = false;
                static bool f4Pressed = false;
                
                if (ImGui::Checkbox("Enable Alt+F4 to close game", &enableAltF4)) {
                    // Reset key states when toggling
                    altPressed = false;
                    f4Pressed = false;
                }
                
                // Check for Alt+F4 combination
                if (enableAltF4) {
                    if (GetAsyncKeyState(VK_MENU) & 0x8000) { // ALT key
                        altPressed = true;
                    } else if (altPressed) {
                        // If ALT was released, check for F4
                        if (GetAsyncKeyState(VK_F4) & 0x8000) { // F4 key
                            f4Pressed = true;
                        } else if (f4Pressed) {
                            // If F4 was pressed and released after ALT, close the game
                            f4Pressed = false;
                            HWND hwnd = FindWindowA(0, "Rocket League");
                            if (hwnd) {
                                PostMessage(hwnd, WM_CLOSE, 0, 0);
                            }
                        }
                    } else {
                        f4Pressed = false;
                    }
                    
                    // Reset if no keys are pressed
                    if (!(GetAsyncKeyState(VK_MENU) & 0x8000) && !(GetAsyncKeyState(VK_F4) & 0x8000)) {
                        altPressed = false;
                        f4Pressed = false;
                    }
                }

                ImGui::EndTabItem();
'@

# Perform the replacement
$newContent = $content -replace [regex]::Escape($target), $replacement

# Write the updated content back to the file
[System.IO.File]::WriteAllText($filePath, $newContent)

Write-Host "File updated successfully."
