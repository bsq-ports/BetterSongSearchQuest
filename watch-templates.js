

var chokidar = require('chokidar');
var exec = require('child_process').exec;
var fs = require('fs');
var path = require('path');

var watcher = chokidar.watch('assets', {
    ignored: /[\/\\]\./,
    persistent: true
    });

watcher
    .on('add', function(path) {
        console.log('File', path, 'has been added');
        pushFile(path);
    })
    .on('change', function(path) {
        console.log('File', path, 'has been changed');
        pushFile(path);
    })
    .on('unlink', function(path) {
        console.log('File', path, 'has been removed');
    });

function pushFile(file) {
    var filename = path.basename(file);
    var destination = '/sdcard/bsml/BetterSongSearch/' + filename;
    exec('adb push ' + file + ' ' + destination, function(error, stdout, stderr) {
        if (error) {
            console.log('Error pushing file: ' + error);
        }
        else {
            console.log('Pushed ' + file + ' to ' + destination);
        }
    });
}