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
  File /oname=autotrace.exe "../../autotrace.exe"
  File /oname=iconv.dll "3rdparty/iconv.dll"
  File /oname=libffi-8.dll "3rdparty/libffi-8.dll"
!If ${FLAVOUR} == "win32"
  File /oname=libgcc_s_dw2-1.dll "3rdparty/libgcc_s_dw2-1.dll"
!EndIf
  File /oname=libglib-2.0-0.dll "3rdparty/libglib-2.0-0.dll"
  File /oname=libgobject-2.0-0.dll "3rdparty/libgobject-2.0-0.dll"
  File /oname=libintl-8.dll  "3rdparty/libintl-8.dll"
  File /oname=libpcre2-8-0.dll "3rdparty/libpcre2-8-0.dll"
  File /oname=libpng16-16.dll "3rdparty/libpng16-16.dll"
!If ${FLAVOUR} == "win32"
  File /oname=libwinpthread-1.dll "3rdparty/libwinpthread-1.dll"
!EndIf
  File /oname=zlib1.dll "3rdparty/zlib1.dll"

SectionEnd ; end the section
