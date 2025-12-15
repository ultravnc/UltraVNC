// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2001 HorizonLive.com, Inc. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


//
// Clipboard frame.
//

import java.awt.*;
import java.awt.event.*;

class ClipboardFrame extends Frame
  implements WindowListener, ActionListener {

  TextArea textArea;
  Button clearButton, closeButton;
  String selection;
  VncViewer viewer;

  //
  // Constructor.
  //

  ClipboardFrame(VncViewer v) {
    super("UltraVNC Clipboard");

    viewer = v;

    GridBagLayout gridbag = new GridBagLayout();
    setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.fill = GridBagConstraints.BOTH;
    gbc.weighty = 1.0;

    textArea = new TextArea(5, 40);
    gridbag.setConstraints(textArea, gbc);
    add(textArea);

    gbc.fill = GridBagConstraints.HORIZONTAL;
    gbc.weightx = 1.0;
    gbc.weighty = 0.0;
    gbc.gridwidth = 1;

    clearButton = new Button("Clear");
    gridbag.setConstraints(clearButton, gbc);
    add(clearButton);
    clearButton.addActionListener(this);

    closeButton = new Button("Close");
    gridbag.setConstraints(closeButton, gbc);
    add(closeButton);
    closeButton.addActionListener(this);

    pack();

    addWindowListener(this);
  }


  //
  // Set the cut text from the RFB server.
  //

  void setCutText(String text) {
    selection = text;
    textArea.setText(text);
    if (isVisible()) {
      textArea.selectAll();
    }
  }


  //
  // When the focus leaves the window, see if we have new cut text and
  // if so send it to the RFB server.
  //

  public void windowDeactivated (WindowEvent evt) {
    if (selection != null && !selection.equals(textArea.getText())) {
      selection = textArea.getText();
      viewer.setCutText(selection);
    }
  }

  //
  // Close our window properly.
  //

  public void windowClosing(WindowEvent evt) {
    setVisible(false);
  }

  //
  // Ignore window events we're not interested in.
  //

  public void windowActivated(WindowEvent evt) {}
  public void windowOpened(WindowEvent evt) {}
  public void windowClosed(WindowEvent evt) {}
  public void windowIconified(WindowEvent evt) {}
  public void windowDeiconified(WindowEvent evt) {}


  //
  // Respond to button presses
  //

  public void actionPerformed(ActionEvent evt) {
    if (evt.getSource() == clearButton) {
      textArea.setText("");
    } else if (evt.getSource() == closeButton) {
      setVisible(false);
    }
  }
}
