/* Logger Pro Wine compatibility: an advertisement watcher without a radio.
 * No devices or advertisements are fabricated. Registered only in the LoggerPro bottle.
 */
typedef unsigned long ULONG;
typedef long HRESULT;
typedef unsigned long long U64;
#define CALL __attribute__((stdcall))
#define EXPORT
#define OK 0
#define NOINTERFACE ((HRESULT)0x80004002)
typedef struct { unsigned long a; unsigned short b,c; unsigned char d[8]; } GUID;
typedef struct { void **vt; } Object;
static int same(const GUID *a,const GUID *b) { const unsigned char *x=(const void*)a,*y=(const void*)b; for(int i=0;i<16;i++) if(x[i]!=y[i]) return 0; return 1; }
static const GUID unknown={0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID inspectable={0xaf86e2e0,0xb12d,0x4c6a,{0x9c,0x5a,0xd7,0xaa,0x65,0x10,0x1e,0x90}};
static const GUID agile={0x94ea2b94,0xe9cc,0x49e0,{0xc0,0xff,0xee,0x64,0xca,0x8f,0x5b,0x90}};
static const GUID activation={0x35,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID watcher_id={0xa6ac336f,0xf3d3,0x4297,{0x8d,0x6c,0xc8,0x1e,0xa6,0x62,0x3f,0x40}};
static Object watcher,factory,signal_filter;
static const GUID signal_id={0xdf7b7391,0x6bb5,0x4cfe,{0x90,0xb1,0x5d,0x73,0x24,0xed,0xcf,0x7f}};
static const GUID static_id_0={0x6dc57058,0x9aba,0x4417,{0xb8,0xf2,0xdc,0xe0,0x16,0xd3,0x4e,0xe2}};
static Object static_obj_0;
static const GUID static_id_1={0xd2ae94f5,0x3d15,0x4f79,{0x9c,0xc,0xea,0xaf,0xa6,0x75,0x15,0x5c}};
static Object static_obj_1;
static const GUID static_id_2={0x58fa4586,0xb1de,0x470c,{0xb7,0xde,0xd,0x11,0xff,0x44,0xf4,0xb7}};
static Object static_obj_2;
static const GUID static_id_3={0x1855b425,0xd46e,0x4a2c,{0x9c,0x3f,0xed,0x6d,0xea,0x29,0xe7,0xbe}};
static Object static_obj_3;
static HRESULT CALL query(Object *self,const GUID *id,void **out) {
 *out=0;
if(same(id,&static_id_0)) { *out=&static_obj_0; return OK; }
if(same(id,&static_id_1)) { *out=&static_obj_1; return OK; }
if(same(id,&static_id_2)) { *out=&static_obj_2; return OK; }
if(same(id,&static_id_3)) { *out=&static_obj_3; return OK; }
 if(same(id,&unknown)||same(id,&inspectable)||same(id,&agile)||(self==&factory&&same(id,&activation))||(self==&watcher&&same(id,&watcher_id))||(self==&signal_filter&&same(id,&signal_id))) { *out=self; return OK; }
 return NOINTERFACE;
}
static ULONG CALL ref(Object *s) { return 2; }
static HRESULT CALL ids(Object*s,ULONG*n,GUID**v) { *n=0; *v=0; return OK; }
static HRESULT CALL name(Object*s,void**v) { *v=0; return OK; }
static HRESULT CALL trust(Object*s,int*v) { *v=0; return OK; }
static HRESULT CALL activate(Object*s,void**v) { *v=&watcher; return OK; }
static HRESULT CALL span(Object*s,U64*v) { *v=1000000; return OK; }
static int status,mode;
static HRESULT CALL get_status(Object*s,int*v) { *v=status; return OK; }
static HRESULT CALL get_mode(Object*s,int*v) { *v=mode; return OK; }
static HRESULT CALL put_mode(Object*s,int v) { mode=v; return OK; }
static HRESULT CALL get_filter(Object*s,void**v) { *v=0; return OK; }
static HRESULT CALL get_signal(Object*s,void**v) { *v=&signal_filter; return OK; }
static HRESULT CALL put_filter(Object*s,void*v) { return OK; }
static HRESULT CALL start(Object*s) { status=1; return OK; }
static HRESULT CALL stop(Object*s) { status=3; return OK; }
static HRESULT CALL add(Object*s,void*h,U64*t) { *t=1; return OK; }
static HRESULT CALL remove_handler(Object*s,U64 t) { return OK; }
static void *watcher_vt[]={query,ref,ref,ids,name,trust,span,span,span,span,get_status,get_mode,put_mode,get_signal,put_filter,get_filter,put_filter,start,stop,add,remove_handler,add,remove_handler};
static void *factory_vt[]={query,ref,ref,ids,name,trust,activate};
static void *signal_vt[]={query,ref,ref,ids,name,trust,get_filter,put_filter,get_filter,put_filter,get_filter,put_filter,get_filter,put_filter};
static Object signal_filter={signal_vt};
static Object watcher={watcher_vt},factory={factory_vt};
EXPORT HRESULT CALL DllGetActivationFactory(void *class_id,void **out) { *out=&factory; return OK; }

static HRESULT CALL uuid_0_Battery(Object*s,GUID*v) { static const GUID g={0x180f,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_BloodPressure(Object*s,GUID*v) { static const GUID g={0x1810,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_CyclingSpeedAndCadence(Object*s,GUID*v) { static const GUID g={0x1816,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_GenericAccess(Object*s,GUID*v) { static const GUID g={0x1800,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_GenericAttribute(Object*s,GUID*v) { static const GUID g={0x1801,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_Glucose(Object*s,GUID*v) { static const GUID g={0x1808,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_HealthThermometer(Object*s,GUID*v) { static const GUID g={0x1809,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_HeartRate(Object*s,GUID*v) { static const GUID g={0x180d,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_0_RunningSpeedAndCadence(Object*s,GUID*v) { static const GUID g={0x1814,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static void *static_vt_0[]={query,ref,ref,ids,name,trust,uuid_0_Battery,uuid_0_BloodPressure,uuid_0_CyclingSpeedAndCadence,uuid_0_GenericAccess,uuid_0_GenericAttribute,uuid_0_Glucose,uuid_0_HealthThermometer,uuid_0_HeartRate,uuid_0_RunningSpeedAndCadence};
static Object static_obj_0={static_vt_0};
static HRESULT CALL uuid_1_AlertNotification(Object*s,GUID*v) { static const GUID g={0x1811,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_CurrentTime(Object*s,GUID*v) { static const GUID g={0x2a2b,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_CyclingPower(Object*s,GUID*v) { static const GUID g={0x1818,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_DeviceInformation(Object*s,GUID*v) { static const GUID g={0x180a,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_HumanInterfaceDevice(Object*s,GUID*v) { static const GUID g={0x1812,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_ImmediateAlert(Object*s,GUID*v) { static const GUID g={0x1802,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_LinkLoss(Object*s,GUID*v) { static const GUID g={0x1803,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_LocationAndNavigation(Object*s,GUID*v) { static const GUID g={0x1819,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_NextDstChange(Object*s,GUID*v) { static const GUID g={0x1807,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_PhoneAlertStatus(Object*s,GUID*v) { static const GUID g={0x180e,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_ReferenceTimeUpdate(Object*s,GUID*v) { static const GUID g={0x1806,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_ScanParameters(Object*s,GUID*v) { static const GUID g={0x1813,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_1_TxPower(Object*s,GUID*v) { static const GUID g={0x1804,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static void *static_vt_1[]={query,ref,ref,ids,name,trust,uuid_1_AlertNotification,uuid_1_CurrentTime,uuid_1_CyclingPower,uuid_1_DeviceInformation,uuid_1_HumanInterfaceDevice,uuid_1_ImmediateAlert,uuid_1_LinkLoss,uuid_1_LocationAndNavigation,uuid_1_NextDstChange,uuid_1_PhoneAlertStatus,uuid_1_ReferenceTimeUpdate,uuid_1_ScanParameters,uuid_1_TxPower};
static Object static_obj_1={static_vt_1};
static HRESULT CALL uuid_2_BatteryLevel(Object*s,GUID*v) { static const GUID g={0x2a19,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_BloodPressureFeature(Object*s,GUID*v) { static const GUID g={0x2a49,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_BloodPressureMeasurement(Object*s,GUID*v) { static const GUID g={0x2a35,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_BodySensorLocation(Object*s,GUID*v) { static const GUID g={0x2a38,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_CscFeature(Object*s,GUID*v) { static const GUID g={0x2a5c,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_CscMeasurement(Object*s,GUID*v) { static const GUID g={0x2a5b,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_GlucoseFeature(Object*s,GUID*v) { static const GUID g={0x2a51,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_GlucoseMeasurement(Object*s,GUID*v) { static const GUID g={0x2a18,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_GlucoseMeasurementContext(Object*s,GUID*v) { static const GUID g={0x2a34,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_HeartRateControlPoint(Object*s,GUID*v) { static const GUID g={0x2a39,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_HeartRateMeasurement(Object*s,GUID*v) { static const GUID g={0x2a37,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_IntermediateCuffPressure(Object*s,GUID*v) { static const GUID g={0x2a36,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_IntermediateTemperature(Object*s,GUID*v) { static const GUID g={0x2a1e,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_MeasurementInterval(Object*s,GUID*v) { static const GUID g={0x2a21,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_RecordAccessControlPoint(Object*s,GUID*v) { static const GUID g={0x2a52,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_RscFeature(Object*s,GUID*v) { static const GUID g={0x2a54,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_RscMeasurement(Object*s,GUID*v) { static const GUID g={0x2a53,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_SCControlPoint(Object*s,GUID*v) { static const GUID g={0x2a55,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_SensorLocation(Object*s,GUID*v) { static const GUID g={0x2a5d,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_TemperatureMeasurement(Object*s,GUID*v) { static const GUID g={0x2a1c,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_2_TemperatureType(Object*s,GUID*v) { static const GUID g={0x2a1d,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static void *static_vt_2[]={query,ref,ref,ids,name,trust,uuid_2_BatteryLevel,uuid_2_BloodPressureFeature,uuid_2_BloodPressureMeasurement,uuid_2_BodySensorLocation,uuid_2_CscFeature,uuid_2_CscMeasurement,uuid_2_GlucoseFeature,uuid_2_GlucoseMeasurement,uuid_2_GlucoseMeasurementContext,uuid_2_HeartRateControlPoint,uuid_2_HeartRateMeasurement,uuid_2_IntermediateCuffPressure,uuid_2_IntermediateTemperature,uuid_2_MeasurementInterval,uuid_2_RecordAccessControlPoint,uuid_2_RscFeature,uuid_2_RscMeasurement,uuid_2_SCControlPoint,uuid_2_SensorLocation,uuid_2_TemperatureMeasurement,uuid_2_TemperatureType};
static Object static_obj_2={static_vt_2};
static HRESULT CALL uuid_3_AlertCategoryId(Object*s,GUID*v) { static const GUID g={0x2a43,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_AlertCategoryIdBitMask(Object*s,GUID*v) { static const GUID g={0x2a42,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_AlertLevel(Object*s,GUID*v) { static const GUID g={0x2a06,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_AlertNotificationControlPoint(Object*s,GUID*v) { static const GUID g={0x2a44,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_AlertStatus(Object*s,GUID*v) { static const GUID g={0x2a3f,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GapAppearance(Object*s,GUID*v) { static const GUID g={0x2a01,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_BootKeyboardInputReport(Object*s,GUID*v) { static const GUID g={0x2a22,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_BootKeyboardOutputReport(Object*s,GUID*v) { static const GUID g={0x2a32,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_BootMouseInputReport(Object*s,GUID*v) { static const GUID g={0x2a33,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_CurrentTime(Object*s,GUID*v) { static const GUID g={0x2a2b,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_CyclingPowerControlPoint(Object*s,GUID*v) { static const GUID g={0x2a66,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_CyclingPowerFeature(Object*s,GUID*v) { static const GUID g={0x2a65,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_CyclingPowerMeasurement(Object*s,GUID*v) { static const GUID g={0x2a63,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_CyclingPowerVector(Object*s,GUID*v) { static const GUID g={0x2a64,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_DateTime(Object*s,GUID*v) { static const GUID g={0x2a08,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_DayDateTime(Object*s,GUID*v) { static const GUID g={0x2a0a,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_DayOfWeek(Object*s,GUID*v) { static const GUID g={0x2a09,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GapDeviceName(Object*s,GUID*v) { static const GUID g={0x2a00,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_DstOffset(Object*s,GUID*v) { static const GUID g={0x2a0d,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ExactTime256(Object*s,GUID*v) { static const GUID g={0x2a0c,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_FirmwareRevisionString(Object*s,GUID*v) { static const GUID g={0x2a26,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_HardwareRevisionString(Object*s,GUID*v) { static const GUID g={0x2a27,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_HidControlPoint(Object*s,GUID*v) { static const GUID g={0x2a4c,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_HidInformation(Object*s,GUID*v) { static const GUID g={0x2a4a,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_Ieee1107320601RegulatoryCertificationDataList(Object*s,GUID*v) { static const GUID g={0x2a2a,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_LnControlPoint(Object*s,GUID*v) { static const GUID g={0x2a6b,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_LnFeature(Object*s,GUID*v) { static const GUID g={0x2a6a,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_LocalTimeInformation(Object*s,GUID*v) { static const GUID g={0x2a0f,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_LocationAndSpeed(Object*s,GUID*v) { static const GUID g={0x2a67,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ManufacturerNameString(Object*s,GUID*v) { static const GUID g={0x2a29,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ModelNumberString(Object*s,GUID*v) { static const GUID g={0x2a24,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_Navigation(Object*s,GUID*v) { static const GUID g={0x2a68,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_NewAlert(Object*s,GUID*v) { static const GUID g={0x2a46,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GapPeripheralPreferredConnectionParameters(Object*s,GUID*v) { static const GUID g={0x2a04,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GapPeripheralPrivacyFlag(Object*s,GUID*v) { static const GUID g={0x2a02,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_PnpId(Object*s,GUID*v) { static const GUID g={0x2a50,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_PositionQuality(Object*s,GUID*v) { static const GUID g={0x2a69,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ProtocolMode(Object*s,GUID*v) { static const GUID g={0x2a4e,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GapReconnectionAddress(Object*s,GUID*v) { static const GUID g={0x2a03,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ReferenceTimeInformation(Object*s,GUID*v) { static const GUID g={0x2a14,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_Report(Object*s,GUID*v) { static const GUID g={0x2a4d,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ReportMap(Object*s,GUID*v) { static const GUID g={0x2a4b,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_RingerControlPoint(Object*s,GUID*v) { static const GUID g={0x2a40,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_RingerSetting(Object*s,GUID*v) { static const GUID g={0x2a41,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ScanIntervalWindow(Object*s,GUID*v) { static const GUID g={0x2a4f,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_ScanRefresh(Object*s,GUID*v) { static const GUID g={0x2a31,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_SerialNumberString(Object*s,GUID*v) { static const GUID g={0x2a25,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_GattServiceChanged(Object*s,GUID*v) { static const GUID g={0x2a05,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_SoftwareRevisionString(Object*s,GUID*v) { static const GUID g={0x2a28,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_SupportedNewAlertCategory(Object*s,GUID*v) { static const GUID g={0x2a47,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_SupportUnreadAlertCategory(Object*s,GUID*v) { static const GUID g={0x2a48,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_SystemId(Object*s,GUID*v) { static const GUID g={0x2a23,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeAccuracy(Object*s,GUID*v) { static const GUID g={0x2a12,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeSource(Object*s,GUID*v) { static const GUID g={0x2a13,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeUpdateControlPoint(Object*s,GUID*v) { static const GUID g={0x2a16,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeUpdateState(Object*s,GUID*v) { static const GUID g={0x2a17,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeWithDst(Object*s,GUID*v) { static const GUID g={0x2a11,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TimeZone(Object*s,GUID*v) { static const GUID g={0x2a0e,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_TxPowerLevel(Object*s,GUID*v) { static const GUID g={0x2a07,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static HRESULT CALL uuid_3_UnreadAlertStatus(Object*s,GUID*v) { static const GUID g={0x2a45,0,0x1000,{0x80,0,0,0x80,0x5f,0x9b,0x34,0xfb}}; *v=g; return OK; }
static void *static_vt_3[]={query,ref,ref,ids,name,trust,uuid_3_AlertCategoryId,uuid_3_AlertCategoryIdBitMask,uuid_3_AlertLevel,uuid_3_AlertNotificationControlPoint,uuid_3_AlertStatus,uuid_3_GapAppearance,uuid_3_BootKeyboardInputReport,uuid_3_BootKeyboardOutputReport,uuid_3_BootMouseInputReport,uuid_3_CurrentTime,uuid_3_CyclingPowerControlPoint,uuid_3_CyclingPowerFeature,uuid_3_CyclingPowerMeasurement,uuid_3_CyclingPowerVector,uuid_3_DateTime,uuid_3_DayDateTime,uuid_3_DayOfWeek,uuid_3_GapDeviceName,uuid_3_DstOffset,uuid_3_ExactTime256,uuid_3_FirmwareRevisionString,uuid_3_HardwareRevisionString,uuid_3_HidControlPoint,uuid_3_HidInformation,uuid_3_Ieee1107320601RegulatoryCertificationDataList,uuid_3_LnControlPoint,uuid_3_LnFeature,uuid_3_LocalTimeInformation,uuid_3_LocationAndSpeed,uuid_3_ManufacturerNameString,uuid_3_ModelNumberString,uuid_3_Navigation,uuid_3_NewAlert,uuid_3_GapPeripheralPreferredConnectionParameters,uuid_3_GapPeripheralPrivacyFlag,uuid_3_PnpId,uuid_3_PositionQuality,uuid_3_ProtocolMode,uuid_3_GapReconnectionAddress,uuid_3_ReferenceTimeInformation,uuid_3_Report,uuid_3_ReportMap,uuid_3_RingerControlPoint,uuid_3_RingerSetting,uuid_3_ScanIntervalWindow,uuid_3_ScanRefresh,uuid_3_SerialNumberString,uuid_3_GattServiceChanged,uuid_3_SoftwareRevisionString,uuid_3_SupportedNewAlertCategory,uuid_3_SupportUnreadAlertCategory,uuid_3_SystemId,uuid_3_TimeAccuracy,uuid_3_TimeSource,uuid_3_TimeUpdateControlPoint,uuid_3_TimeUpdateState,uuid_3_TimeWithDst,uuid_3_TimeZone,uuid_3_TxPowerLevel,uuid_3_UnreadAlertStatus};
static Object static_obj_3={static_vt_3};
