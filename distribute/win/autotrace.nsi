!ifndef VERSION
  !error "VERSION must be defined!"
!endif

!ifndef FLAVOUR
  !error "FLAVOUR must be defined!"
!endif

!ifndef DLLPATH
  !error "DLLPATH must be defined!"
!endif

Name "AutoTrace"
AllowRootDirInstall true
Caption "AutoTrace ${VERSION} - convert bitmaps into vector graphics"
ShowInstDetails show

Icon "autotrace.ico"
OutFile "autotrace-${VERSION}-${FLAVOUR}-setup.exe"

InstallDir $LOCALAPPDATA\AutoTrace
RequestExecutionLevel user

Page directory
Page instfiles

Section "" ;No components page, name is not important
  SetOutPath $INSTDIR

  File /oname=autotrace.exe "../../.libs/autotrace.exe"

  File "${DLLPATH}/iconv.dll"
  File "${DLLPATH}/libffi-8.dll"
!if ${FLAVOUR} == "win32"
  File "${DLLPATH}/libgcc_s_dw2-1.dll"
!else
  File "${DLLPATH}/libgcc_s_seh-1.dll"
!endif
  File "${DLLPATH}/libglib-2.0-0.dll"
  File "${DLLPATH}/libgobject-2.0-0.dll"
  File "${DLLPATH}/libintl-8.dll"
  File "${DLLPATH}/libpcre2-8-0.dll"
  File "${DLLPATH}/libpng16-16.dll"
  File "${DLLPATH}/libwinpthread-1.dll"
  File "${DLLPATH}/zlib1.dll"

SectionEnd ; end the section
