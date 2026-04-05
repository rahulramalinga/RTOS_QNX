@    #50.builtin
@    setconf TIMEZONE __OPT_TZ__
@    mount -uo noexec /dev/shmem
@    rtc hw
@# local/snippets/ifs_start.custom
@# Commands executed from ifs start-up script prior to invoking main start-up script or slm
@    #~30.builtin
#if defined(OPT_SLM_yes)
@    START_P(slm_t) slm -Vvv __IFS_PATH__/slm.cfg &
#else
@    __IFS_PATH__/startup.sh
#endif
@    #~80.builtin
#if defined(OPT_SLM_yes)
@    waitfor /system
#endif
@
@    display_msg "Startup complete"
@    uname -a
@
@    reopen __DEVC_PATH__
#if defined(__MUON_ALT_CONSOLE__)
@    [+session] ksh
#endif
