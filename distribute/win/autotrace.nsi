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
OutFile "autotrace-${VERSION}-${FLAVOUR}-setup.exe"

; The default installation directory
;InstallDir $EXEDIR
; Do not use PROGRAMFILES, that is for 32bit code only!
InstallDir $PROGRAMFILES64\AutoTrace

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
  File /oname=autotrace.exe "../../.libs/autotrace.exe"
!If ${FLAVOUR} == "win64"
  File /oname=libgobject-2.0-0.dll "/usr/local/bin/libgobject-2.0-0.dll"
  File /oname=libglib-2.0-0.dll "/usr/local/bin/libglib-2.0-0.dll"
  File /oname=libffi-7.dll "/usr/local/bin/libffi-7.dll"
  File /oname=libintl-8.dll "/usr/local/bin/libintl-8.dll"
!EndIf
!If ${FLAVOUR} == "win32"
  File /oname=libgobject-2.0-0.dll "/usr/local/bin/libgobject-2.0-0.dll"
  File /oname=libglib-2.0-0.dll "/usr/local/bin/libglib-2.0-0.dll"
  File /oname=libffi-7.dll "/usr/local/bin/libffi-7.dll"
  File /oname=libintl-8.dll "/usr/local/bin/libintl-8.dll"
  File /oname=libgcc_s_dw2-1.dll "/usr/i686-w64-mingw32/bin/libgcc_s_dw2-1.dll"
  File /oname=libwinpthread-1.dll "/usr/i686-w64-mingw32/bin/libwinpthread-1.dll"
!EndIf

SectionEnd ; end the section
