; autotrace.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The installer simply
; prompts the user asking them where to install, and drops a copy of example1.nsi
; there.

;--------------------------------

!ifndef VERSION
  !define VERSION 'anonymous-build'
!endif

; The name of the installer
Name "AutoTrace"
AllowRootDirInstall true
Caption "AutoTrace ${VERSION} - convert bitmaps into vector graphics"
ShowInstDetails show

Icon "autotrace.ico"

; The file to write
OutFile "autotrace-${VERSION}-setup.exe"

; The default installation directory
;InstallDir $EXEDIR
InstallDir $PROGRAMFILES

; Request application privileges for Windows Vista
; RequestExecutionLevel user
RequestExecutionLevel admin

;--------------------------------

; Pages

Page directory
Page instfiles

;--------------------------------

; The stuff to install
Section "" ;No components page, name is not important

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put file there
  File /oname=autotrace.exe "../../autotrace.exe"

SectionEnd ; end the section
