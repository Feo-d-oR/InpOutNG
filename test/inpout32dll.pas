unit inpout32dll;

interface

uses Windows;

type
  PHandle = ^THandle;

type
  PPortTask = ^TPortTask;
  TPortTask = packed record
    taskOperation : Word;
    taskPort      : Word;
    taskData      : LongWord;
  end;

type
  TPortOperation = (
    INPOUT_NOTIFY =         $00,
    INPOUT_READ8 =          $01,
    INPOUT_READ16 =         $02,
    INPOUT_READ32 =         $04,
    INPOUT_WRITE8 =         $08,
    INPOUT_WRITE16 =        $10,
    INPOUT_WRITE32 =        $20,
    INPOUT_ACK =            $80,
    INPOUT_READ8_ACK =      $81,
    INPOUT_READ16_ACK =     $82,
    INPOUT_READ32_ACK =     $84,
    INPOUT_WRITE8_ACK =     $88,
    INPOUT_WRITE16_ACK =    $90,
    INPOUT_WRITE32_ACK =    $a0);

type
  POutPOrtData = ^TOutPortData;
  TOutPortData = packed record
    Addr : Word;
    Val : packed record case byte of
      1 : (outChar : Byte);
      2 : (outShrt : Word);
      3 : (outLong : LongWord);
    end;
  end;

type
  PInPortData = ^TInPortData;
  TInPortData = packed record case byte of
    1 : (inChar : Byte);
    2 : (inShrt : Word);
    3 : (inLong : LongWord);
  end;

// 3 группы функций: inp8/outp8, ...16, ...32, чтобы были однотипные названия.

type // интерфейс библиотеки
  //My extra functions for making life easy
  TDlDrvOpen = function : ByteBool; stdcall; //Returns TRUE if the InpOut driver was opened successfully
  TDlIs64Bit = function : ByteBool; stdcall; //Returns TRUE if the OS is 64bit (x64) Windows.

  //DLLPortIO function support
  TDlInp8 =   function (portAddr : Word) : Byte; stdcall;
  TDlInp16 =  function (portAddr : Word) : Word; stdcall;
  TDlInp32 =  function (portAddr : Word) : LongWord; stdcall;
  TDlOutp8 =  procedure(portAddr:Word; portData : Byte); stdcall;
  TDlOutp16 = procedure(portAddr:Word; portData : Word); stdcall;
  TDlOutp32 = procedure(portAddr:Word; portData : LongWord);stdcall;

  TDlRegNotify = function:UINT;stdcall;
  TDlGetNotify = function:UINT;stdcall;

  TDlIrqWait = function(inTask     : PPortTask;
                        inTaskSize : LongWord;
                        outTask    : PPortTask;
                        outTaskSize: LongWord) : ByteBool;

  TDlIrqRequest = procedure(inTask      : PPortTask;
                            inTaskSize  : LongWord;
                            outTask     : PPortTask;
                            outTaskSize : LongWord;
                            notifyStatus: PLongWord);

  TDlForceNotify = procedure;

  function DllLoad : Boolean;
  function GetProcFromDll (procName : String) : Pointer;

var
  WM_INPOUTNG_NOTIFY : PUINT; cvar;

  RegNotify  : TDlRegNotify;
  GetNotify  : TDlGetNotify;


  DlIrqWait    : TDlIrqWait;
  DLIrqRequest : TDLIrqRequest;
  DLForceNotify: TDLForceNotify;

implementation

uses SysUtils, Dialogs;

const
  DllName = 'inpout32.dll';

var
  DllHandle : TLibHandle;

function GetProcFromDll(procName: String): Pointer;
begin
  Result:=GetProcedureAddress(dllHandle, procName);
  if (Result = NIL)
  then
    begin
     MessageDlg('Function not found!', Format('Object %s not found in DLL!', [procName]), mtError, [ mbOK ], 'ErrorOnDllFuncSearch');
     Exit;
    end;
end;

function DllLoad:Boolean;
begin
  dllHandle := LoadLibrary(DllName);
  Result:=dllHandle<>NilHandle;
  if dllHandle = NilHandle
  then
    begin
      MessageDlg('Dll not loaded!', 'Could not load DLL!', mtError, [ mbOK ], 'ErrorOnDllLoad');
      Exit;
    end;
  RegNotify     := TDlRegNotify(GetProcFromDll('registerWmNotify'));
  GetNotify     := TDlGetNotify(GetProcFromDll('getWmNotify'));
  DlIrqWait     := TDlIrqWait(GetProcFromDll('waitForIrq'));
  DlIrqRequest  := TDlIrqRequest(GetProcFromDll('requestIrqNotify'));
  DlForceNotify := TDlForceNotify(GetProcFromDll('forceNotify'));

end;

end.
