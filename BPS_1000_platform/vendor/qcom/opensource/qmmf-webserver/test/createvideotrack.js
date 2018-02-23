/*
* Copyright (c) 2016, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
var system = require('system');

function createvideotrack(args) {
    var create_url = "/createvideotrack";
    var create_args = {
        operation: "POST",
        encoding: "utf8",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded",
            "X-Requested-With": "XMLHttpRequest",
            "Accept": "application/json, text/javascript, */*; q=0.01",
            "Connection": "keep-alive",
        },
        data: "camera_id="
    };
    create_args.data += args[2];
    create_args.data += "&track_id=" + args[3];
    create_args.data += "&session_id=" + args[4];
    create_args.data += "&track_width=" + args[5];
    create_args.data += "&track_height=" + args[6];
    create_args.data += "&framerate=" + args[7];
    create_args.data += "&track_codec=" + args[8];
    create_args.data += "&track_output=" + args[9];
    create_args.data += "&bitrate=" + args[10];
    create_args.data += "&low_power_mode=" + args[11];
    var create_page = require('webpage').create();
    var url = args[1] + create_url;
    create_page.open(url, create_args, function(status) {
        var reply = JSON.parse(create_page.plainText);
        console.log("request status: " + reply.Status);
        console.log("request error: " + reply.Error);
        phantom.exit();
    });
}

if (system.args.length < 12) {
    console.log("Usage: <server-url> <camera_id> <track_id> <session_id> <width> <height> <framerate> <track_codec> <track_output> <bitrate> <low_power_mode>");
    phantom.exit(1);
} else {
    createvideotrack(system.args);
}
