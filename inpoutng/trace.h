/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

--*/

//
// Define the tracing flags.
//
// Tracing GUID - aecbb916-711d-4b21-918f-43a488c5ef24
//

#define WPP_CONTROL_GUIDS                                               \
    WPP_DEFINE_CONTROL_GUID(                                            \
        inpOutNgTraceGuid, (aecbb916,711d,4b21,918f,43a488c5ef24),      \
                                                                        \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                    \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                    \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                     \
        WPP_DEFINE_BIT(TRACE_IOCTL)                                     \
        WPP_DEFINE_BIT(TRACE_IRQ)                                       \
        WPP_DEFINE_BIT(TRACE_IRQ_DPC)                                   \
        WPP_DEFINE_BIT(TRACE_INIT)                                      \
        WPP_DEFINE_BIT(TRACE_CONFIG)                                    \
        WPP_DEFINE_BIT(TRACE_ASYNC)                                     \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
//begin_wpp config
//FUNC Trace{FLAGS=TRACE_DRIVER}(LEVEL, MSG, ...);
//FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
//end_wpp
//
