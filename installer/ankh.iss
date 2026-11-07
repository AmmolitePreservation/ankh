#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef NumericVersion
  #error NumericVersion is required
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#ifndef RepositoryRoot
  #error RepositoryRoot is required
#endif

[Setup]
AppId={{6E1F4C3A-5B7D-4E0A-9C21-A47D2B90E6F1}
AppName=Ankh
AppVersion={#AppVersion}
AppPublisher=AmmolitePreservation
AppPublisherURL=https://github.com/AmmolitePreservation/ankh
AppSupportURL=https://github.com/AmmolitePreservation/ankh/issues
VersionInfoVersion={#NumericVersion}
DefaultDirName={autopf}\Ankh
DefaultGroupName=Ankh
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=ankhSetup-{#AppVersion}-win64
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern dark
WizardImageFile={#RepositoryRoot}\assets\installerSide.bmp
WizardSmallImageFile={#RepositoryRoot}\assets\installerSmall.bmp
SetupIconFile={#RepositoryRoot}\assets\ankh.ico
UninstallDisplayIcon={app}\ankh.exe
CloseApplications=yes

#define VCRedistFile StageDir + "\vc_redist.x64.exe"
#define VCRedistVersion GetVersionNumbersString(VCRedistFile)

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "vc_redist.x64.exe"; Flags: recursesubdirs ignoreversion
Source: "{#VCRedistFile}"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: NeedsVCRedist

[Icons]
Name: "{autoprograms}\Ankh"; Filename: "{app}\ankh.exe"

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing the Microsoft Visual C++ runtime..."; Flags: shellexec waituntilterminated; Check: NeedsVCRedist
Filename: "{app}\ankh.exe"; Description: "Launch Ankh"; Flags: nowait postinstall skipifsilent
Filename: "{app}\ankh.exe"; Flags: nowait runasoriginaluser; Check: IsRelaunchRequested

[Code]
function IsRelaunchRequested: Boolean;
begin
  Result := ExpandConstant('{param:RELAUNCH|0}') = '1';
end;

function NeedsVCRedist: Boolean;
var
  Major, Minor, Bld, Rbld: Cardinal;
  Installed, Required: Int64;
  Key: String;
begin
  Key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  Result := True;
  if RegQueryDWordValue(HKLM64, Key, 'Major', Major) and RegQueryDWordValue(HKLM64, Key, 'Minor', Minor)
    and RegQueryDWordValue(HKLM64, Key, 'Bld', Bld) and RegQueryDWordValue(HKLM64, Key, 'Rbld', Rbld) then
  begin
    if not StrToVersion('{#VCRedistVersion}', Required) then
      RaiseException('The bundled Visual C++ runtime has an unreadable version.');
    if not StrToVersion(Format('%d.%d.%d.%d', [Major, Minor, Bld, Rbld]), Installed) then
      RaiseException('The installed Visual C++ runtime has an unreadable version.');
    Result := ComparePackedVersion(Installed, Required) < 0;
  end;
end;
