Unicode true
!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

Name "Boss Rally"
OutFile "${OUTPUT}"
InstallDir "$PROGRAMFILES32\SouthPeak Interactive\Boss Rally"
InstallDirRegKey HKLM "SOFTWARE\SouthPeak Interactive\Boss Rally" "Directory"
RequestExecutionLevel admin

Var DiscPath
Var DiscInput

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
Page custom DiscPage DiscPageLeave
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function DiscPage
  nsDialogs::Create 1018
  Pop $0
  ${NSD_CreateLabel} 0 0 100% 28u \
    "Select the original Boss Rally disc directory containing BossRally.pod and BossRally.vdb."
  Pop $0
  ${NSD_CreateText} 0 36u 82% 13u "$EXEDIR"
  Pop $DiscInput
  ${NSD_CreateBrowseButton} 84% 36u 16% 13u "Browse..."
  Pop $0
  ${NSD_OnClick} $0 BrowseDisc
  nsDialogs::Show
FunctionEnd

Function BrowseDisc
  nsDialogs::SelectFolderDialog "Original Boss Rally disc" "$EXEDIR"
  Pop $0
  ${If} $0 != "error"
    ${NSD_SetText} $DiscInput $0
  ${EndIf}
FunctionEnd

Function DiscPageLeave
  ${NSD_GetText} $DiscInput $DiscPath
  ${IfNot} ${FileExists} "$DiscPath\BossRally.pod"
    MessageBox MB_ICONSTOP "BossRally.pod was not found in that directory."
    Abort
  ${EndIf}
  ${IfNot} ${FileExists} "$DiscPath\BossRally.vdb"
    MessageBox MB_ICONSTOP "BossRally.vdb was not found in that directory."
    Abort
  ${EndIf}
FunctionEnd

Section "Boss Rally" MainSection
  SetRegView 32
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File "${BUILD}/src/BRD3D.dll"
  File "${GLIDE_BUILD}/src/BRGlide.dll"
  File "${BUILD}/src/win32/ear/earpds.dll"
  File "${BUILD}/src/win32/boot/BRally.exe"
  File "${BUILD}/src/win32/boot/BossRally.exe"
  File "${BUILD}/src/win32/boot/SetVideo.exe"

  ClearErrors
  CopyFiles /SILENT "$DiscPath\BossRally.pod" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\BossRally.vdb" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\BRString.dll" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\Boss Rally Web Site.url" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\brally.avi" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\loading.img" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\splash.img" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\RallyIntro1.dat" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\RallyIntro2.dat" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\RallyCredits.dat" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\RallyOutro.dat" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\ReadMe.txt" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\cargfx" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\cars" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\Images" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\Paint" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\sfx" "$INSTDIR"
  CopyFiles /SILENT "$DiscPath\tracks" "$INSTDIR"
  ${If} ${Errors}
    MessageBox MB_ICONSTOP "Could not copy the original Boss Rally data."
    Abort
  ${EndIf}

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "SOFTWARE\SouthPeak Interactive\Boss Rally" "Directory" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Boss Rally" \
    "DisplayName" "Boss Rally"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Boss Rally" \
    "UninstallString" '"$INSTDIR\Uninstall.exe"'
  CreateDirectory "$SMPROGRAMS\Boss Rally"
  CreateShortcut "$SMPROGRAMS\Boss Rally\Boss Rally.lnk" "$INSTDIR\BossRally.exe"
  CreateShortcut "$SMPROGRAMS\Boss Rally\Boss Rally (no intro).lnk" "$INSTDIR\BRally.exe"
  CreateShortcut "$SMPROGRAMS\Boss Rally\Boss Rally Display Wizard.lnk" \
    "$INSTDIR\SetVideo.exe"
  CreateShortcut "$SMPROGRAMS\Boss Rally\Visit SouthPeak.lnk" \
    "$INSTDIR\Boss Rally Web Site.url"
  CreateShortcut "$SMPROGRAMS\Boss Rally\ReadMe.lnk" "$WINDIR\Notepad.exe" \
    '"$INSTDIR\ReadMe.txt"'
  CreateShortcut "$SMPROGRAMS\Boss Rally\Uninstall Boss Rally.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  SetRegView 32
  SetShellVarContext all
  Delete "$SMPROGRAMS\Boss Rally\Boss Rally.lnk"
  Delete "$SMPROGRAMS\Boss Rally\Boss Rally (no intro).lnk"
  Delete "$SMPROGRAMS\Boss Rally\Boss Rally Display Wizard.lnk"
  Delete "$SMPROGRAMS\Boss Rally\Visit SouthPeak.lnk"
  Delete "$SMPROGRAMS\Boss Rally\ReadMe.lnk"
  Delete "$SMPROGRAMS\Boss Rally\Uninstall Boss Rally.lnk"
  RMDir "$SMPROGRAMS\Boss Rally"
  DeleteRegKey HKLM "SOFTWARE\SouthPeak Interactive\Boss Rally"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Boss Rally"
  Delete "$INSTDIR\BRD3D.dll"
  Delete "$INSTDIR\BRGlide.dll"
  Delete "$INSTDIR\earpds.dll"
  Delete "$INSTDIR\BRally.exe"
  Delete "$INSTDIR\BossRally.exe"
  Delete "$INSTDIR\SetVideo.exe"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
SectionEnd
