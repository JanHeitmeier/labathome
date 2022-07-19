Library LabAtHome;

{$R *.res}

uses
  WinProcs,
  WinTypes,
  SysUtils,
  Controls,
  Forms,
  Classes,
  Graphics,
  StdCtrls,
  Dialogs,
  Sockets, Math,
  IdBaseComponent, IdComponent, IdUDPBase, IdUDPClient, IdGlobal,
  LabAtHomeDlg in 'LabAtHomeDlg.pas' {LabAtHomeDialog};

type
  PUserData = ^TUserData;
  TUserData = record
    FileBuildNr: integer;
    IPAddress: string;
    Port: integer;
    UDP: TIdUDPClient;
    IsInputConnected: array[1..50] of boolean;
  end;

  PParameterStruct=^TParameterStruct;
  TParameterStruct=packed record
    NuE : Byte;                           {Anzahl reeller Zahlenwerte}
    NuI : Byte;                           {Anzahl ganzer Zahlenwerte}
    NuB : Byte;                           {Anzahl Schalter}
    E:Array[0..31] of Extended;           {reelle Zahlenwerte}
    I:Array[0..31] of Integer;            {ganze Zahlenwerte}
    B:Array[0..31] of Byte;               {Schalter}
    D:Array[0..255] of AnsiChar;          {event. Dateiname f�r weitere Daten.}
    EMin:Array[0..31] of Extended;        {untere Eingabegrenze f�r jeden reellen Zahlenwert}
    EMax:Array[0..31] of Extended;        {obere Eingabegrenze f�r jeden rellen Zahlenwert}
    IMin:Array[0..31] of Integer;         {untere Eingabegrenze f�r jeden ganzzahligen Zahlenwert}
    IMax:Array[0..31] of Integer;         {obere Eingabegrenze f�r jeden ganzzahligen Zahlenwert}
    NaE : Array[0..31,0..40] of AnsiChar; {Namen der reellen Zahlenwerten}
    NaI : Array[0..31,0..40] of AnsiChar; {Namen der ganzen Zahlenwerten}
    NaB : Array[0..31,0..40] of AnsiChar; {Namen der Schalter}
    UserDataPtr: PUserData;               {Zeiger auf weitere Blockvariablen}
    ParentPtr: Pointer;                   {Zeiger auf User-DLL-Block}
    ParentHWnd: HWnd;                     {Handle des User-DLL-Blocks}
    ParentName: PAnsiChar;                {Name des User-DLL-Blocks}
    UserHWindow: HWnd;                    {Benutzerdef. Fensterhandle, z. B. f�r Ausgabefenster}
    DataFile: text;                       {Textdatei f�r universelle Zwecke}
  end;

  PDialogEnableStruct=^TDialogEnableStruct;
  TDialogEnablestruct=packed record
    AllowE: Longint;                      { Soll die Eingabe eines Wertes   }
    AllowI: Longint;                      { un-/zul�ssig sein so ist das Bit}
    AllowB: Longint;                      { des Allow?-Feldes 0 bzw. 1}
    AllowD: Byte;
  end;

  PNumberOfInputsOutputs=^TNumberOfInputsOutputs;
  TNumberOfInputsOutputs=packed record
    Inputs :Byte;                         {Anzahl Eing�nge}
    Outputs:Byte;                         {Anzahl Ausg�nge}
    NameI : Array[0..49,0..40] of AnsiChar;
    NameO : Array[0..49,0..40] of AnsiChar;
  end;

  PInputArray = ^TInputArray;
  TInputArray = packed array[1..30] of extended;
  POutputArray = ^TOutputArray;
  TOutputArray = packed array[1..30] of extended;

  PConnectedArray = ^TConnectedArray;
  TConnectedArray = packed Array[1..50] of byte;


function GetVersionString: string;
var n, Len: cardinal;
    Buf, Value: PChar;
    s: array[0..1000] of Char;
begin
  GetModuleFileName(HInstance, s, 1000);
  n := GetFileVersionInfoSize(s, n);
  Buf := AllocMem(n);
  GetFileVersionInfo(s, 0, n, Buf);
  VerQueryValue(Buf, PChar('StringFileInfo\040704E4\FileVersion'), Pointer(Value), Len);
  Result := StrPas(Value);
  FreeMem(Buf, n);
end;

function GetVersionBuild(Version: string): integer;
begin
  while Pos('.', Version) > 0 do delete(Version, 1, pos('.', Version));
  Result := StrToInt(Version);
end;

procedure SetEnhancedInformation2(DeltaT, TSimu: extended; D1: PConnectedArray; D2: PParameterStruct); export stdcall;
var i: integer;
begin
  for i:=1 to 50 do
    D2.UserDataPtr.IsInputConnected[i] := D1[i] > 0;
end;


procedure GetParameterStruct(D:PParameterStruct);export stdcall;
begin
  {Anzahl der Parameter}
  D^.NuE := 0;
  D^.NuI := 1;
  D^.NuB := 0;
  D^.I[0] := 0;
end;

procedure GetDialogEnableStruct(D:PDialogEnableStruct;D2:PParameterStruct);export stdcall;
begin
  {Alle Dialogelemente sollen jederzeit zug�nglich sein!}
  D^.AllowE:=$FFFFFFFF;
  D^.AllowB:=$FFFFFFFF;
  D^.AllowI:=$FFFFFFFF;
  D^.AllowD:= 1;
end;

procedure GetNumberOfInputsOutputs2(D:PNumberofInputsOutputs; Parameterfilename: PChar;
                                    UserDataPtr: Pointer; UserHWindow: Pointer);export stdcall;
var i : Integer;
begin
  if UserDataPtr <> nil then begin
    D^.Inputs := 14;
    D^.Outputs := 18;
    StrPCopy(D^.NameI[0], 'Relay K3 [0/5]');
    StrPCopy(D^.NameI[1], 'Soll-Sound [integer]');
    StrPCopy(D^.NameI[2], 'Soll-Versorgungsspannung am USB-C [0-65535]');
    StrPCopy(D^.NameI[3], 'Leistung Heizwiderstand [Prozent als float]');
    StrPCopy(D^.NameI[4], 'Helligkeit Power-LED [Prozent als float]');
    StrPCopy(D^.NameI[5], 'Soll-Geschwindigkeit FAN1 [Prozent als float]');
    StrPCopy(D^.NameI[6], 'Soll-Geschwindigkeit FAN2 [Prozent als float]');
    StrPCopy(D^.NameI[7], 'Soll-Position Servo 1 [Winkel als float]');
    StrPCopy(D^.NameI[8], 'Soll-Position Servo 2 [Winkel als float]');
    StrPCopy(D^.NameI[9], 'Analoger Ausgang 0-10V [Volt als float]');
    StrPCopy(D^.NameI[10], 'LED0 [Farbe als RGB-Integer]');
    StrPCopy(D^.NameI[11], 'LED1 [Farbe als RGB-Integer]');
    StrPCopy(D^.NameI[12], 'LED2 [Farbe als RGB-Integer]');
    StrPCopy(D^.NameI[13], 'LED3 [Farbe als RGB-Integer]');




    StrPCopy(D^.NameO[0], 'Taster rot [0/5]');
    StrPCopy(D^.NameO[1], 'Taster gelb [0/5]');
    StrPCopy(D^.NameO[2], 'Taster gr�n [0/5]');
    StrPCopy(D^.NameO[3], 'Bewegungsmelder [0/5]');
    StrPCopy(D^.NameO[4], 'Drehencoder-Z�hlwert [Schritte als integer]');
    StrPCopy(D^.NameO[5], 'Ist-Sound [integer]');
    StrPCopy(D^.NameO[6], 'Ist-Versorgungsspannung am USB-C [Volt als float]');
    StrPCopy(D^.NameO[7], 'Ist-Helligkeit [Lux als float]');
    StrPCopy(D^.NameO[8], 'Temperatur des Heizwiderstandes [�C als float]');
    StrPCopy(D^.NameO[9], 'Raumtemperatur [�C als float]');
    StrPCopy(D^.NameO[10], 'Raum-Luftdruck [pa als float]');
    StrPCopy(D^.NameO[11], 'Raum-Luftfeuchtigkeit [rel.% als float]');
    StrPCopy(D^.NameO[12], 'Raum-CO2 [ppm als float]');
    StrPCopy(D^.NameO[13], 'Raum-Luftqualit�t [Prozent als float]');
    StrPCopy(D^.NameO[14], 'Externer Luftdruck [pa als float]');
    StrPCopy(D^.NameO[15], 'Signalst�rke des WiFi-AccessPoints [RSSI dB als float]');
    StrPCopy(D^.NameO[16], 'Externer Spannungseingang [Volt als float]');
    StrPCopy(D^.NameO[17], 'Ist-Geschwindigkeit FAN1 [rpm als float]');
  end;
end;

function CanSimulateDLL(D:PParameterStruct):Integer; export stdcall;
begin
  Result := 1;
end;

procedure SimulateDLL2(T, DeltaT:Extended;D1:PParameterStruct;Inputs:PInputArray;Outputs:POutputArray);export stdcall;
var i: integer;
    s: string;
    B: TIdBytes;
    incrementalValid:Boolean;
    soundValid:Boolean;
    offset:integer;
procedure SetFloat(InputIndex: integer);
var tmp:Single;
begin
  if not D1^.UserDataPtr.IsInputConnected[InputIndex] then
    tmp:=nan
  Else
    tmp:=Inputs^[InputIndex];
  Move(tmp, B[offset], 4);
  offset:=offset+4;
end;
procedure SetByte(InputIndex: integer);
begin
  if not D1^.UserDataPtr.IsInputConnected[InputIndex] Then
    B[offset]:=255
  Else if Inputs^[InputIndex]<2.5 then
    B[offset]:=1
  else
    B[offset]:=0;
  offset:=offset+1;
end;

procedure SetValidSign(InputIndex: integer);
var i:integer;
begin
  if not D1^.UserDataPtr.IsInputConnected[InputIndex] Then
    B[offset]:=0
  else
    B[offset]:=1;
  offset:=offset+1;
end;

procedure SetIntegerRaw(i: integer);

begin
  B[offset+0] := (i shr  0) and $000000FF;
  B[offset+1] := (i shr  8) and $000000FF;
  B[offset+2] := (i shr 16) and $000000FF;
  B[offset+3] := (i shr 24) and $000000FF;
  offset:=offset+4;
end;

procedure SetInteger(InputIndex: integer);
var i:integer;
begin
  SetIntegerRaw(round(Inputs^[InputIndex]));
end;

function ParseInteger():integer;
begin
  Move(B[offset], Result, 4);
  offset:=offset+4;
end;

function ParseValid():Boolean;
begin
  if B[offset]=1 then
    Result:=True
  else
    Result:=False;
  offset:=offset+1;
end;

procedure ParseAndSetInteger(OutputIndex: integer; reallySet:Boolean);
var tmp:integer;
begin
  tmp:=ParseInteger();
  if reallySet Then
    Outputs^[OutputIndex] := tmp;
end;

procedure ParseAndSetFloat(OutputIndex: integer);
var tmp:Single;
begin
  Move(B[offset], tmp, 4);
  offset:=offset+4;
  if not IsNaN(tmp) Then
    Outputs^[OutputIndex] := tmp;
end;


procedure ParseAndSetBool(OutputIndex: integer);
begin
  if B[offset]= 0 then
    Outputs^[OutputIndex] := 0
  else if B[offset] < 255 then
    Outputs^[OutputIndex] := 5;
  //else do not set anything, als value is not valid
  offset:=offset+1;
end;
begin
  offset:=0;
  //Aufbau der MessageOutputData-Nachricht
  SetLength(B, 60);
  SetIntegerRaw(2); //MessageID
  SetByte(1);       // Relay K3 [0/5]:
  SetValidSign(2);  // Soll-Sound
  offset:=offset+2; // padding
  SetInteger(2);    // Soll-Sound
  SetFloat(3);      // Soll-Versorgungsspannung am USB-C
  SetFloat(4);      // Leistung Heizwiderstand [0-65535]:
  SetFloat(5);      // Helligkeit Power-LED [0-65535]:
  SetFloat(6);      // Soll-Geschwindigkeit FAN1 [0-65535]:
  SetFloat(7);      // Soll-Geschwindigkeit FAN2 [0-65535]:
  SetFloat(8);      // Soll-Position Servo 1 [0-65535]:
  SetFloat(9);      // Soll-Position Servo 2 [0-65535]:
  SetFloat(10);     // Analoger Ausgang 0-10V [0-65535]:
  SetInteger(11);    // LED0 [RGB]
  SetInteger(12);    // LED1 [RGB]
  SetInteger(13);    // LED2 [RGB]
  SetInteger(14);    // LED3 [RGB]


  D1^.UserDataPtr.UDP.SendBuffer(B);
  offset:=0;
  try
    SetLength(B, 0);
    SetLength(B, 68);
    D1^.UserDataPtr.UDP.ReceiveBuffer(B, 200);
    if(ParseInteger()<>3 ) then //MessageType==3!!!
      exit;
    // Taster rot [0/5]:
    ParseAndSetBool(1);
    // Taster gelb [0/5]:
    ParseAndSetBool(2);
    // Taster gr�n [0/5]:
    ParseAndSetBool(3);
    // Bewegungsmelder [0/5]:
    ParseAndSetBool(4);
    incrementalValid:=ParseValid();
    soundValid:=ParseValid();
    offset:=offset+2; // padding
    ParseAndSetInteger(5, incrementalValid);// Drehencoder-Z�hlwert [integer]:
    ParseAndSetInteger(6, soundValid); // Ist-Sound [integer]:
    ParseAndSetFloat(7);// Ist-Versorgungsspannung am USB-C [Volt als float]:
    ParseAndSetFloat(8);  // Ist-Helligkeit [Lux als float]:
    ParseAndSetFloat(9);  // Temperatur des Heizwiderstandes [�C als Float]:
    ParseAndSetFloat(10); // Raumtemperatur [�C als float]:
    ParseAndSetFloat(11);  // Raum-Luftdruck [pa als float]:
    ParseAndSetFloat(12); // Raum-Luftfeuchtigkeit [%rel als float]:
    ParseAndSetFloat(13);  // Raum-CO2 [ppm als float]:
    ParseAndSetFloat(14);    // Raum-Luftqualit�t [% als Float]:
    ParseAndSetFloat(15);    // AirSpeedMeterPerSecond
    ParseAndSetFloat(16);    // WifiSignalStrengthDB
    ParseAndSetFloat(17);    // AnalogInputVolt
    ParseAndSetFloat(18);    // Fan1RotationsRpM

  except

  end;
end;

procedure InitSimulationDLL(D1:PParameterStruct;Inputs:PInputArray;Outputs:POutputArray);export stdcall;
begin
  D1^.UserDataPtr.UDP := TIdUDPClient.Create(nil);
try
  D1^.UserDataPtr.UDP.Host := D1^.UserDataPtr.IPAddress;
  D1^.UserDataPtr.UDP.Port := D1^.UserDataPtr.Port;
  D1^.UserDataPtr.UDP.Active := true;
except
end;
  SimulateDLL2(0, 0, D1, Inputs, Outputs);
end;

procedure InitUserDLL(D: PParameterStruct); export stdcall;
var i: integer;
begin
  Application.Handle := D^.ParentHWnd;
  D^.UserDataPtr := new(PUserData);
  with D^.UserDataPtr^ do begin
    IPAddress := '127.0.0.1';
    Port := 1600;
  end;
end;

procedure DisposeUserDLL(D: PParameterStruct); export stdcall;
begin
  Application.Handle := 0;
  Dispose(D^.UserDataPtr);
end;

function GetDLLName: PAnsiChar; export stdcall;
begin
  GetDLLName := 'lab@home';
end;

procedure CallParameterDialogDLL(D1:PParameterStruct; D2:PNumberOfInputsOutputs);export stdcall;
(* Stellt den benutzerdefinierten Parameterdialog dar *)
var Dialog: TLabAtHomeDialog;
    i: integer;
begin
  Dialog := TLabAtHomeDialog.Create(Application);
  with Dialog, D1^.UserDataPtr^ do begin
    IPEdit.Text := IPAddress;
    PortEdit.Text := IntToStr(Port);
    if ShowModal = mrOK then begin
      IPAddress := IPEdit.Text;
      try
        Port := StrToInt(PortEdit.Text);
      except
        Port := 1600;
      end;
    end;
  end;
  Dialog.Free;
end;

Procedure WriteToFile(AFileHandle:Word; D: PParameterStruct); export stdcall;
var i: integer;
    s: array[0..1000] of AnsiChar;
begin
  with D^.UserDataPtr^ do begin
    Str(GetVersionBuild(GetVersionString), s); StrCat(s, #13#10); _lWrite(AFileHandle, s, StrLen(s)); // Build-Nummer der DLL
    StrPCopy(s, IPAddress + #13#10); _lWrite(AFileHandle, s, StrLen(s));
    StrPCopy(s, IntToStr(Port) + #13#10); _lWrite(AFileHandle, s, StrLen(s));
  end;
end;

Procedure ReadFromFile(AFileHandle:Word; D: PParameterStruct); export stdcall;
var i, Code: integer;
    s: array[0..1000] of AnsiChar;
procedure ReadOneLine(FHandle:Word; Aps :PAnsiChar);
var i  : Integer;
begin
  i:=0;
  _lRead(FHandle,@Aps[i],1);
  repeat
    inc(i);
    _lRead(FHandle,@Aps[i],1);
  until (Aps[i-1]=#13) and (Aps[i]=#10);
  Aps[i-1]:=#0;
end;
begin
  ReadOneLine(AFileHandle, s);
  D^.UserDataPtr.FileBuildNr := StrToInt(StrPas(s)); // Build-Nummer der DLL;
  ReadOneLine(AFileHandle, s); D^.UserDataPtr.IPAddress := StrPas(s);
  ReadOneLine(AFileHandle, s); D^.UserDataPtr.Port := StrToInt(StrPas(s));
end;

procedure EndSimulationDLL2(D: PParameterStruct);export stdcall;
begin
  D^.UserDataPtr.UDP.Active := false;
  D^.UserDataPtr.UDP.Free;
end;

procedure IsUserDLL32; export stdcall;
begin
end;

procedure IsDemoDLL; export stdcall;
begin
end;

{Exportieren der notwendigen Funktionen und Prozeduren }
exports
  GetParameterStruct,
  GetDialogEnableStruct,
  GetNumberOfInputsOutputs2,
  CanSimulateDLL,
  InitSimulationDLL,
  SimulateDLL2,
  InitUserDLL,
  DisposeUserDLL,
  GetDLLName,
  CallParameterDialogDLL,
  WriteToFile,
  ReadFromFile,
  EndSimulationDLL2,
  SetEnhancedInformation2,
  IsDemoDLL,
  IsUserDLL32;

begin
  {Weitere Initialisierung der DLL}
  FormatSettings.DecimalSeparator := '.';
  Application.UpdateFormatSettings := false;
end.
