object PasswordDialog: TPasswordDialog
  Left = 0
  Top = 0
  AutoSize = True
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'UltraVNC Manager'
  ClientHeight = 256
  ClientWidth = 517
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poScreenCenter
  TextHeight = 15
  object Panel1: TPanel
    Left = 0
    Top = 188
    Width = 517
    Height = 59
    Align = alTop
    BevelOuter = bvNone
    Caption = 'Panel1'
    TabOrder = 0
    object Button2: TButton
      AlignWithMargins = True
      Left = 189
      Top = 16
      Width = 128
      Height = 35
      Margins.Left = 24
      Margins.Top = 16
      Margins.Right = 24
      Margins.Bottom = 8
      Align = alRight
      Caption = 'Cancel'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -14
      Font.Name = 'Segoe UI'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      OnClick = Button2Click
    end
    object ButtonOK: TButton
      AlignWithMargins = True
      Left = 365
      Top = 16
      Width = 128
      Height = 35
      Margins.Left = 24
      Margins.Top = 16
      Margins.Right = 24
      Margins.Bottom = 8
      Align = alRight
      Caption = 'Set password'
      Default = True
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -14
      Font.Name = 'Segoe UI'
      Font.Style = []
      ModalResult = 1
      ParentFont = False
      TabOrder = 1
      OnClick = ButtonOKClick
    end
  end
  object EditOldPass: TEdit
    AlignWithMargins = True
    Left = 24
    Top = 16
    Width = 469
    Height = 27
    Margins.Left = 24
    Margins.Top = 16
    Margins.Right = 24
    Margins.Bottom = 4
    Align = alTop
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -14
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    PasswordChar = '*'
    TabOrder = 1
    TextHint = 'Old password'
  end
  object EditNewPass: TEdit
    AlignWithMargins = True
    Left = 24
    Top = 63
    Width = 469
    Height = 27
    Margins.Left = 24
    Margins.Top = 16
    Margins.Right = 24
    Margins.Bottom = 4
    Align = alTop
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -14
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    PasswordChar = '*'
    TabOrder = 2
    TextHint = 'New password'
    OnChange = EditNewPassChange
  end
  object InfoText: TStaticText
    AlignWithMargins = True
    Left = 24
    Top = 157
    Width = 469
    Height = 23
    Margins.Left = 24
    Margins.Top = 16
    Margins.Right = 24
    Margins.Bottom = 8
    Align = alTop
    BevelInner = bvNone
    Caption = 
      'After setting a password the ini file is encrypted, the file can' +
      #39't be unencrypted without the proper password.'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -14
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    TabOrder = 3
  end
  object EditNewPass2: TEdit
    AlignWithMargins = True
    Left = 24
    Top = 110
    Width = 469
    Height = 27
    Margins.Left = 24
    Margins.Top = 16
    Margins.Right = 24
    Margins.Bottom = 4
    Align = alTop
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -14
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    PasswordChar = '*'
    TabOrder = 4
    TextHint = 'New password again'
    OnChange = EditNewPassChange
  end
end
