unit frmmain;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, Buttons, StdCtrls;

type
  PHandle = ^THandle;

type // интерфейс библиотеки
  TDlOut32 = procedure (portAddr : SmallInt; portData : SmallInt); stdcall;
  TDlInp32 = function  (portAddr : SmallInt) : SmallInt; stdcall;

  //My extra functions for making life easy
  TDlDrvOpen = function : ByteBool; stdcall; //Returns TRUE if the InpOut driver was opened successfully
  TDlIs64Bit = function : ByteBool; stdcall; //Returns TRUE if the OS is 64bit (x64) Windows.

  //DLLPortIO function support
  TDlReadPortUchar   = function (portAddr : Word) : Byte; stdcall;
  TDlReadPortUshort  = function (portAddr : Word) : Word; stdcall;
  TDlReadPortUlong   = function (portAddr : LongWord) : LongWord; stdcall;
  TDlWritePortUchar  = procedure (portAddr:Word; portData : Byte); stdcall;
  TDlWritePortUshort = procedure (portAddr:Word; portData : Word); stdcall;
  TDlWritePortUlong  = procedure (portAddr:LongWord; portData : LongWord);stdcall;

//WinIO function support (Untested and probably does NOT work - esp. on x64!)
  TDlMapPhysToLin = function (pbPhysAddr : PByte; dwPhysSize : DWord; pPhysicalMemoryHandle: PHandle):PByte; stdcall;
  TDlUnmapPhysicalMemory = function(PhysicalMemoryHandle : THandle; pbLinAddr : PByte):ByteBool;stdcall;
  TDlGetPhysLong = function(pbPhysAddr : PByte; pdwPhysVal : PDWord) : ByteBool; stdcall;
  TDlSetPhysLong = function(pbPhysAddr : PByte; dwPhysVal : DWord) : ByteBool; stdcall;

type

  { TForm1 }

  TForm1 = class(TForm)
    eNumRuns: TEdit;
    btnRunTest: TSpeedButton;
    procedure btnRunTestClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
    dllHandle : TLibHandle;
    DlOut32 : TDlOut32;
    DlInp32 : TDlInp32;
    DlDrvOpen : TDlDrvOpen;
    DlIs64Bit : TDlIs64Bit;

    DlReadPortUchar   : TDlReadPortUchar;
    DlReadPortUshort  : TDlReadPortUshort;
    DlReadPortUlong   : TDlReadPortUlong;
    DlWritePortUchar  : TDlWritePortUchar;
    DlWritePortUshort : TDlWritePortUshort;
    DlWritePortUlong  : TDlWritePortUlong;

    DlMapPhysToLin        : TDlMapPhysToLin;
    DlUnmapPhysicalMemory : TDlUnmapPhysicalMemory;
    DlGetPhysLong         : TDlGetPhysLong;
    DlSetPhysLong         : TDlSetPhysLong;
  private
    function GetProcFromDll (ADllHandle : THandle; procName : String) : Pointer;
    function FunctionsFound : Boolean;
  public

  end;

var
  Form1: TForm1;

const
  DllName = 'inpout32.dll';

implementation

uses libTime;

{$R *.lfm}

{ TForm1 }
procedure TForm1.FormCreate(Sender: TObject);
begin
  dllHandle := LoadLibrary(DllName);
  if dllHandle = NilHandle
  then
    begin
      MessageDlg('Dll not loaded!', 'Could not load DLL!', mtError, [ mbOK ], 'ErrorOnDllLoad');
      Exit;
    end;
  DlOut32 := TDlOut32(GetProcFromDll(dllHandle, 'Out32'));
  DlInp32 := TDlInp32(GetProcFromDll(dllHandle, 'Inp32'));

  DlDrvOpen := TDlDrvOpen(GetProcFromDll(dllHandle, 'IsInpOutDriverOpen'));
  DlIs64Bit := TDlIs64Bit(GetProcFromDll(dllHandle, 'IsXP64Bit'));

  DlReadPortUchar  :=  TDlReadPortUchar(  GetProcFromDll(dllHandle, 'DlPortReadPortUchar'));
  DlReadPortUshort :=  TDlReadPortUshort( GetProcFromDll(dllHandle, 'DlPortReadPortUshort'));
  DlReadPortUlong  :=  TDlReadPortUlong(  GetProcFromDll(dllHandle, 'DlPortReadPortUlong'));
  DlWritePortUchar :=  TDlWritePortUchar( GetProcFromDll(dllHandle, 'DlPortWritePortUchar'));
  DlWritePortUshort := TDlWritePortUshort(GetProcFromDll(dllHandle, 'DlPortWritePortUshort'));
  DlWritePortUlong :=  TDlWritePortUlong( GetProcFromDll(dllHandle, 'DlPortWritePortUlong'));

  DlMapPhysToLin :=        TDlMapPhysToLin( GetProcFromDll(dllHandle, 'MapPhysToLin'));
  DlUnmapPhysicalMemory := TDlUnmapPhysicalMemory(GetProcFromDll(dllHandle, 'UnmapPhysicalMemory'));
  DlGetPhysLong := TDlGetPhysLong( GetProcFromDll(dllHandle, 'GetPhysLong'));
  DlSetPhysLong := TDlSetPhysLong( GetProcFromDll(dllHandle, 'SetPhysLong'));

  MessageDlg(Format('Everything is %s!', [BoolToStr(FunctionsFound, 'OK', 'crap')]),
             Format('Dll is %s and is %s bits', [ BoolToStr(DlDrvOpen(), 'loaded', 'NOT loaded'), BoolToStr(DlIs64Bit(), '64', '32')]),
             mtInformation,
             [mbOK],
             'FormLoadComplete');
end;

procedure TForm1.btnRunTestClick(Sender: TObject);
var
  TStart : double = 0;
  TEnd : double = 0;
  dt : double = 0;
  numCycles : Integer = 0;
  i : integer = 0;
  Val : SmallInt;
begin
  TryStrToInt(eNumRuns.Text, numCycles);
  if numCycles<=0
  then begin ShowMessage('numCycles not recognized'); Exit; end;
  TStart:=GetUNIXTime;
  for i:=1 to numCycles do
  begin
    Val:=DlInp32($300);
    DlOut32($300, Val);
  end;
  TEnd:=GetUNIXTime;
  dt:=TEnd-TStart;
  ShowMessage(Format('%d cycles passed!%sTimespent = %.6f sec (%.6f per r/w cycle)',
                     [numCycles, LineEnding, dt, dt/numCycles]));
end;

function TForm1.GetProcFromDll(ADllHandle: THandle; procName: String): Pointer;
begin
  Result := NIL;
  if ADllHandle = NilHandle
  then
    begin
     MessageDlg('Dll is NIL!', 'Null pointer supplied!', mtError, [ mbOK ], 'ErrorOnDllHandlePass');
     Exit;
    end;
  Result:=GetProcedureAddress(dllHandle, procName);
  if (Result = NIL)
  then
    begin
     MessageDlg('Function not found!', Format('Object %s not found in DLL!', [procName]), mtError, [ mbOK ], 'ErrorOnDllFuncSearch');
     Exit;
    end;
end;

function TForm1.FunctionsFound: Boolean;
begin
  Result := Assigned(DlOut32) and Assigned(DlInp32) and
            Assigned(DlDrvOpen) and Assigned(DlIs64Bit) and
            Assigned(DlReadPortUchar) and Assigned(DlWritePortUchar) and
            Assigned(DlReadPortUshort) and Assigned(DlWritePortUshort) and
            Assigned(DlReadPortUlong) and Assigned(DlWritePortUlong) and
            Assigned(DlMapPhysToLin) and Assigned(DlUnmapPhysicalMemory) and
            Assigned(DlGetPhysLong) and Assigned(DlSetPhysLong);
end;

end.

