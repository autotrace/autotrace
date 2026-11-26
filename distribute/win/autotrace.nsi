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

; The installer is per user (InstallDir is under $LOCALAPPDATA), so its
; entry in Windows' list of installed apps goes under HKCU.
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\AutoTrace"
; An upgrade goes where the last install went, so that it replaces that
; copy instead of leaving it behind without an entry.  /D= still wins.
InstallDirRegKey HKCU "${UNINST_KEY}" "InstallLocation"

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

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

  WriteUninstaller "$INSTDIR\uninstall.exe"

  WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "AutoTrace"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "AutoTrace contributors"
  WriteRegStr HKCU "${UNINST_KEY}" "URLInfoAbout" "https://github.com/autotrace/autotrace"
  WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\autotrace.exe"
  WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1

SectionEnd ; end the section

; Remove exactly the files the installer wrote, then the directory only if
; it is empty: it may be a directory the user chose with /D and keeps other
; files in.
Section "Uninstall"
  Delete "$INSTDIR\autotrace.exe"
  Delete "$INSTDIR\iconv.dll"
  Delete "$INSTDIR\libffi-8.dll"
!if ${FLAVOUR} == "win32"
  Delete "$INSTDIR\libgcc_s_dw2-1.dll"
!else
  Delete "$INSTDIR\libgcc_s_seh-1.dll"
!endif
  Delete "$INSTDIR\libglib-2.0-0.dll"
  Delete "$INSTDIR\libgobject-2.0-0.dll"
  Delete "$INSTDIR\libintl-8.dll"
  Delete "$INSTDIR\libpcre2-8-0.dll"
  Delete "$INSTDIR\libpng16-16.dll"
  Delete "$INSTDIR\libwinpthread-1.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"

  ; A later install into another directory took over the entry: leave it to
  ; that copy.  StrCmp ignores case, as Windows paths do.
  ReadRegStr $0 HKCU "${UNINST_KEY}" "InstallLocation"
  StrCmp $0 $INSTDIR 0 keep_entry
  DeleteRegKey HKCU "${UNINST_KEY}"
keep_entry:
SectionEnd
