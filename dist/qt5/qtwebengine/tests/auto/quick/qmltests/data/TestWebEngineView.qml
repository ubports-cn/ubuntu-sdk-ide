/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0
import QtWebEngine 1.2
import QtWebEngine.experimental 1.0

WebEngineView {
    property var loadStatus: null
    property var viewportReady: false
    property bool windowCloseRequestedSignalEmitted: false

    function waitForLoadSucceeded() {
        var success = _waitFor(function() { return loadStatus == WebEngineView.LoadSucceededStatus })
        loadStatus = null
        return success
    }
    function waitForViewportReady() {
        // Note: You need to have "when: windowShown" in your TestCase for this to work.
        // The viewport is locked until the first frame is rendered, and the rendering isn't
        // activated until the WebView is visible in a mapped QQuickView.
        return _waitFor(function() { return viewportReady })
    }
    function waitForLoadFailed() {
        var failure = _waitFor(function() { return loadStatus == WebEngineView.LoadFailedStatus })
        loadStatus = null
        return failure
    }
    function waitForLoadStopped() {
        var stop = _waitFor(function() { return loadStatus == WebEngineView.LoadStoppedStatus })
        loadStatus = null
        return stop
    }
    function waitForWindowCloseRequested() {
        return _waitFor(function() { return windowCloseRequestedSignalEmitted; });
    }
    function _waitFor(predicate) {
        var timeout = 5000
        var i = 0
        while (i < timeout && !predicate()) {
            testResult.wait(50)
            i += 50
        }
        return predicate()
    }

    TestResult { id: testResult }

    onLoadingChanged: {
        loadStatus = loadRequest.status
        if (loadRequest.status == WebEngineView.LoadStartedStatus)
            viewportReady = false
    }

    onWindowCloseRequested: {
        windowCloseRequestedSignalEmitted = true;
    }
}

