// this function verifies disk space in kilobytes
function verifyDiskSpace(dirPath, spaceRequired)
{
  var spaceAvailable;

  // Get the available disk space on the given path
  spaceAvailable = fileGetDiskSpaceAvailable(dirPath);

  // Convert the available disk space into kilobytes
  spaceAvailable = parseInt(spaceAvailable / 1024);

  // do the verification
  if(spaceAvailable < spaceRequired)
  {
    logComment("Insufficient disk space: " + dirPath);
    logComment("  required : " + spaceRequired + " K");
    logComment("  available: " + spaceAvailable + " K");
    return(false);
  }

  return(true);
}

// this function deletes a file if it exists
function deleteThisFile(dirKey, file)
{
  var fFileToDelete;

  fFileToDelete = getFolder(dirKey, file);
  logComment("File to delete: " + fFileToDelete);
  if(File.isFile(fFileToDelete))
  {
    File.remove(fFileToDelete);
    return(true);
  }
  else
    return(false);
}

// this function deletes a folder if it exists
function deleteThisFolder(dirKey, folder, recursiveDelete)
{
  var fToDelete;

  if(typeof recursiveDelete == "undefined")
    recursiveDelete = true;

  fToDelete = getFolder(dirKey, folder);
  logComment("folder to delete: " + fToDelete);
  if(File.isDirectory(fToDelete))
  {
    File.dirRemove(fToDelete, recursiveDelete);
    return(true);
  }
  else
    return(false);
}

// OS type detection
// which platform?
function getPlatform()
{
  var platformStr;
  var platformNode;

  if('platform' in Install)
  {
    platformStr = new String(Install.platform);

    if (!platformStr.search(/^Macintosh/))
      platformNode = 'mac';
    else if (!platformStr.search(/^Win/))
      platformNode = 'win';
    else if (!platformStr.search(/^OS\/2/))
      platformNode = 'win';
    else
      platformNode = 'unix';
  }
  else
  {
    var fOSMac  = getFolder("Mac System");
    var fOSWin  = getFolder("Win System");

    logComment("fOSMac: "  + fOSMac);
    logComment("fOSWin: "  + fOSWin);

    if(fOSMac != null)
      platformNode = 'mac';
    else if(fOSWin != null)
      platformNode = 'win';
    else
      platformNode = 'unix';
  }

  return platformNode;
}

var err = initInstall("Chatzilla v0.5", "Chatzilla", "1.1.0.0000000000"); 
logComment("initInstall: " + err);

addFile("Chatzilla service",
        "bin/components/chatzilla-service.js",
        getFolder("Components"),
        "");

addFile("Chatzilla Chrome",
        "bin/chrome/chatzilla.jar", // jar source folder 
        getFolder("Chrome"),        // target folder
        "");                        // target subdir 

addDirectory("Chatzilla Icons",
             "bin/chrome/icons",  // jar source folder
             getFolder("Chrome", "icons"), // target folder
             "");                 // target subdir

registerChrome(CONTENT | DELAYED_CHROME, getFolder("Chrome","chatzilla.jar"), "content/chatzilla/");
registerChrome(CONTENT | DELAYED_CHROME, getFolder("Chrome","chatzilla.jar"), "content/chatzilla/sm/");
registerChrome(SKIN | DELAYED_CHROME, getFolder("Chrome","chatzilla.jar"), "skin/modern/chatzilla/");
registerChrome(LOCALE | DELAYED_CHROME, getFolder("Chrome","chatzilla.jar"), "locale/en-US/chatzilla/");

err = getLastError();
if (err==SUCCESS)
    performInstall(); 
else
    cancelInstall(err);

