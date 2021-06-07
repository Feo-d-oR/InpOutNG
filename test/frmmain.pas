unit frmmain;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, Buttons, StdCtrls;

type
  PHandle = ^THandle;

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
type

  { TForm1 }

  TForm1 = class(TForm)
    eNumRuns: TEdit;
    btnRunTest: TSpeedButton;
    procedure btnRunTestClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
    dllHandle : TLibHandle;
    DlDrvOpen : TDlDrvOpen;
    DlIs64Bit : TDlIs64Bit;

    DlInp8   : TDlInp8;
    DlInp16  : TDlInp16;
    DlInp32  : TDlInp32;
    DlOutp8  : TDlOutp8;
    DlOutp16 : TDlOutp16;
    DlOutp32 : TDlOutp32;
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
  DlDrvOpen := TDlDrvOpen(GetProcFromDll(dllHandle, 'IsInpOutDriverOpen'));
  DlIs64Bit := TDlIs64Bit(GetProcFromDll(dllHandle, 'IsXP64Bit'));

  DlInp8   := TDlInp8(  GetProcFromDll(dllHandle, 'inp8'));
  DlInp16  := TDlInp16( GetProcFromDll(dllHandle, 'inp16'));
  DlInp32  := TDlInp32( GetProcFromDll(dllHandle, 'inp32'));
  DlOutp8  := TDlOutp8( GetProcFromDll(dllHandle, 'outp8'));
  DlOutp16 := TDlOutp16(GetProcFromDll(dllHandle, 'outp16'));
  DlOutp32 := TDlOutp32(GetProcFromDll(dllHandle, 'outp32'));

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
    Val:=DlInp16($300);
    DlOutp16($300, Val);
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
  Result := Assigned(DlDrvOpen) and Assigned(DlIs64Bit) and
            Assigned(DlInp8)    and Assigned(DlOutp8)   and
            Assigned(DlInp16)   and Assigned(DlOutp16)  and
            Assigned(DlInp32)   and Assigned(DlOutp32);
end;

end.

