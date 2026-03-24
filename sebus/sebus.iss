[Setup]
AppId={{8B3F2A1C-4D5E-4F6A-8B9C-0D1E2F3A4B5C}
AppName=Sebus Engine + GUI
AppVersion=1.2
DefaultDirName={autopf}\Sebus
DefaultGroupName=Sebus Engine + GUI
AllowNoIcons=yes
OutputDir=C:\Users\allab\OneDrive\Desktop\sebus
OutputBaseFilename=Sebus_Setup
SetupIconFile=C:\Users\allab\OneDrive\Desktop\sebus\icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Types]
Name: "full"; Description: "Full Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "engine"; Description: "Sebus Engine (Core Runner)"; Types: full
Name: "gui"; Description: "Sebus GUI (File Launcher)"; Types: full

[Files]
; Shared Dependencies
Source: "C:\Users\allab\OneDrive\Desktop\sebus\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\allab\OneDrive\Desktop\sebus\icon.ico"; DestDir: "{app}"; Flags: ignoreversion

; Executables
Source: "C:\Users\allab\OneDrive\Desktop\sebus\Sebus.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "C:\Users\allab\OneDrive\Desktop\sebus\SebusGUI.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: gui

[Icons]
Name: "{group}\Sebus Engine"; Filename: "{app}\Sebus.exe"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"; Components: engine
Name: "{autodesktop}\Sebus Engine"; Filename: "{app}\Sebus.exe"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"; Components: engine; Tasks: desktopicon

Name: "{group}\Sebus GUI"; Filename: "{app}\SebusGUI.exe"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"; Components: gui
Name: "{autodesktop}\Sebus GUI"; Filename: "{app}\SebusGUI.exe"; WorkingDir: "{app}"; IconFilename: "{app}\icon.ico"; Components: gui; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\SebusGUI.exe"; Description: "Launch Sebus GUI"; Flags: nowait postinstall skipifsilent; Components: gui
Filename: "{app}\Sebus.exe"; Description: "Launch Sebus Engine"; Flags: nowait postinstall skipifsilent; Components: engine; Check: not IsComponentSelected('gui')