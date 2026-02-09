// Define global variables
let currentPath = "/"; // current directory path
let fileList = document.getElementById("file-list"); // file list DOM element
let fileItems = []; // list of file item DOM elements
let selectedFile = ""; // name of the selected file/folder
let selectedFilePath = ""; // path of the selected file/folder

let scrollDirection = 1; // Standardmäßig positiv für normale Scrollrichtung
let touchStartX = 0; //                                                     
let touchStartY = 0; //                                                     
let mousedownTime = 0; // time when mouse button was pressed down           
let mouseupTime = 0; // time when mouse button was released                 
let clickHandled = false; //                                                
let longClickTimeout; // timeout ID for handling long click events          
let isTouchDevice = false; // flag to indicate if device is touch-enabled   
let touchMoved; //                                                          

let originalPixels; // Declare a global variable to store the original pixel buffer of the image.
let originalPixelsHeight;
let originalPixelsWidth;
let resizedPixels;
let resizedPixelsHeight;
let resizedPixelsWidth;
let currentPixels; // Declare a global variable to store the current pixel buffer of the displayed image.
let currentPixelsWidth = 110;
let currentPixelsHeight = 110;
let canvasWidth = 110; // width of the canvas
let canvasHeight = 110; // height of the canvas
let previewPixels; // Declare a flobal variable to store the preview pixels
let invert = false; // RDC image will be inverted, when true
let rdcFileName; // filename of the RDC image


let imageDataArray;

document.getElementById('xdim').value = canvasWidth;
document.getElementById('ydim').value = canvasHeight;

// Load the list of files and directories in the current path
function loadFileList() {
    console.log('Loading file list...');

    // Send an HTTP GET request to the server to retrieve a list of files and directories in the current path
    fetch(`/filelist?path=${encodeURIComponent(currentPath)}`)
    .then(response => response.json())
    .then(data => {
        console.log('Received file list:', data);

        // Clear any previous elements from the file list display
        fileList.innerHTML = '';

        // If the current path is not the root directory, add a ".." directory to represent the parent directory
        if (currentPath !== "/") {
            const parentItem = document.createElement("div");
            parentItem.textContent = "..";
            parentItem.classList.add("directory");
            parentItem.onclick = () => {
                // Update the path to the parent directory and reload the file list display
                currentPath = currentPath.split('/').slice(0, -2).join('/') + '/';
                loadFileList();
            };
            fileList.appendChild(parentItem);
        }

        // Create DOM elements for each file or directory in the list and add them to the file list display
        const fileItems = data.map(file => {
            const item = document.createElement("div");
            item.textContent = file;

            // Add the "directory" class if it is a directory, to visually highlight it
            if (file.indexOf('.') === -1) {
                item.classList.add("directory");
            }

            // Add a click handler to select the element on click
            item.onclick = () => {
                // Remove the "selected" class from any other elements in the file list
                fileItems.forEach(el => el.classList.remove("selected"));
                // Add the "selected" class to the clicked element
                item.classList.add("selected");
            };
            return item;
        });

        // Add all created file and directory elements to the file list display
        fileItems.forEach(item => fileList.appendChild(item));
    })
    .catch(error => {
        console.error('Error loading file list:', error);
    });

    // Send an HTTP GET request to the server to retrieve information about the available disk space
    fetch('/diskspace')
    .then(response => response.json())
    .then(data => {
        const { totalBytes, usedBytes } = data;
        console.log(`Total Bytes: ${totalBytes}, Used Bytes: ${usedBytes}`);

        // Update the disk space display with the retrieved information
        const diskSpaceDisplay = document.getElementById('diskSpace');
        diskSpaceDisplay.textContent = "SPIFFS total: " + totalBytes + " bytes, free: " + (totalBytes-usedBytes) + " bytes";    
    })
    .catch(error => console.error(error));
}

// Download a file from the server using the specified URL
function downloadFile(fileUrl) {
    console.log("fileUrl = " + fileUrl);
  
    // Send an HTTP GET request to the server to retrieve the file data as a blob object
    fetch('/downloadfile?filename=' + fileUrl)
    .then(response => response.blob())
    .then(blob => {
        var fileName = getFileNameFromUrl(fileUrl); // Extract the filename from the URL
        var link = document.createElement('a');
        link.href = window.URL.createObjectURL(blob);
        link.download = fileName;
        link.click(); // Clicks the "Download" link to download the file
      })
      .catch(function(error) {
        console.log('Problem beim Herunterladen der Datei:', error);
      });
}
  
// Extract the file name from a given URL
function getFileNameFromUrl(url) {
    // Splits the URL by "/" and returns the last element of the resulting array, which is the filename
    var parts = url.split("/");
    return parts[parts.length - 1];
}

// Replaces the file extension of a filename with the extension ".rdc".
function replaceFileExtension(filename) {
    const dotIndex = filename.lastIndexOf('.');
    if (dotIndex !== -1) {
      const basename = filename.substring(0, dotIndex);
      return `${basename}.rdc`;
    } else {
      return `${filename}.rdc`;
    }
}

// Get the size of a file with the specified name
function getFileSize(filename) {
    console.log("getFileSize: " + filename);
  
    return fetch(`/fileSize?filename=${filename}`)
      .then(response => response.json()) // Parse response as JSON
      .then(data => {
        console.log(filename + " fileSize: " + data.size);
        return data.size;
      })
      .catch(error => console.error(error));
}

// Preview the contents of a file with the specified name
function previewFile(fileName) {
    console.log("previewFile: "+fileName);

    // If the file is an image, download it and display it in an <img> element.
    if (/\.(gif|jpe?g|tiff?|png|webp|bmp)$/i.test(fileName)) {
        console.log("previewFile1: "+'/downloadfile?filename=' + encodeURIComponent(fileName));
        fetch('/downloadfile?filename=' + encodeURIComponent(fileName))
        .then(response => response.blob())
        .then(blob => {
            // Hide the textarea and show the image element
            var textareaElement = document.getElementsByTagName("textarea")[0];
            textareaElement.style.display = "none";     
            var imgElement = document.getElementsByTagName("img")[0];
            imgElement.style.display = "block";

            // Create a URL for the blob and set it as the image source
            const objectURL = URL.createObjectURL(blob);
            document.getElementById("image-preview").src = objectURL;
        })
        .catch(error => {
            console.error('Error downloading file:', error);
        });
} 

// If the file is an rdc file, download it and display it in an <img> element.
if (/\.(rdc)$/i.test(fileName)) {
    console.log("previewFile1: " + '/downloadfile?filename=' + encodeURIComponent(fileName));
    fetch('/downloadfile?filename=' + encodeURIComponent(fileName))
      .then(response => response.blob())
      .then(blob => {
        const reader = new FileReader();
        reader.onloadend = function () {
          const arrayBuffer = reader.result;
          const dataView = new DataView(arrayBuffer);
  
          // Create a new Uint8Array to store the file data in the "pixels" array
          const pixels = new Uint8Array(dataView.buffer, dataView.byteOffset, dataView.byteLength);
  
          // Hide the textarea and show the image element
          var textareaElement = document.getElementsByTagName("textarea")[0];
          textareaElement.style.display = "none";
          var imgElement = document.getElementById("image-preview");
          imgElement.style.display = "block";
    
          let x, y, y_byte, y_start, byte_no;
          let xd, yd;

          const firstFourBytes = pixels.slice(0, 4);
          const dataViewForIntX = new DataView(firstFourBytes.buffer);
          xd = dataViewForIntX.getUint32(0, true); // true für little-endian, false für big-endian
          const nextFourBytes = pixels.slice(4, 8);
          const dataViewForIntY = new DataView(nextFourBytes.buffer);
          yd = dataViewForIntY.getUint32(0, true); // true für little-endian, false für big-endian

          console.log("file xdim: "+xd);
          console.log("file ydim: "+yd);  

          let previewTemp = new Uint8Array(xd*yd*4);

          for (x = 0; x < xd; x++) {
            for (y = 0; y < yd; y++) {
              y_byte=Math.floor(3*y/2);
              y_start=(3*y)%2;
              byte_no=12 + x * Math.ceil(yd*1.5) + y_byte;
              const pixelIndex = 4 * x + (yd-1-y) * xd*4;
              let pixelValueR, pixelValueG, pixelValueB;
              if (y_start==0) {
                pixelValueR = 36*(pixels[byte_no]&0x0F);
                pixelValueG = 36*((pixels[byte_no]&0xF0)>>4);
                pixelValueB = 36*(pixels[byte_no+1]&0x0F);
              }
              else {
                pixelValueR = 36*(pixels[byte_no] >> 4);
                pixelValueG = 36*(pixels[byte_no+1] & 0x0F);
                pixelValueB = 36*(pixels[byte_no+1] >>4);
              }

              previewTemp[pixelIndex] = pixelValueR; // Red channel
              previewTemp[pixelIndex + 1] = pixelValueG; // Green channel
              previewTemp[pixelIndex + 2] = pixelValueB; // Blue channel
              previewTemp[pixelIndex + 3] = 255; // Alpha channel (fully opaque)
            }
          }

          // Display the modified image in the HTML element.
          displayRDC('image-preview', previewTemp, xd, yd);
        };
        reader.readAsArrayBuffer(blob);
      })
      .catch(error => {
        console.error('Error downloading file:', error);
      });
  }

  // If the file is a plain text file, download it and display the contents in a <textarea> element.
    if (/\.(txt|css|html|js)$/i.test(fileName)) {
        fetch('/downloadfile?filename=' + encodeURIComponent(fileName))
        .then(response => response.text())
        .then(text => {
            // Hide the image element and show the textarea
            var imgElement = document.getElementsByTagName("img")[0];
            imgElement.style.display = "none";
            var textareaElement = document.getElementsByTagName("textarea")[0];
            textareaElement.style.display = "inline";
            
            // Set the file contents as the textarea value
            document.getElementById("text-box").value = text;
        });
    } 

    // Request the file size and display it.
    if (/\.(gif|jpe?g|tiff?|png|webp|bmp|txt|css|html|js|rdc)$/i.test(fileName)) {
        console.log("request fileSize of " + fileName);

        // Request the file size and create a Promise to handle the result.
        const fileSizePromise = getFileSize(fileName);
        fileSizePromise.then(fileSize => {
            // Update the file size display with the filename and size in bytes.
            const fileSizeDisplay = document.getElementById('fileSize');
            fileSizeDisplay.textContent = getFileNameFromUrl(fileName) + " (" + fileSize + "bytes)";
        }).catch(error => {
            console.error(error);
        });
    }
}

// Clear the currently selected file path
function clearSelectedFilePath() {
  selectedFilePath = "";
  document.getElementById("text-selected-file").value = selectedFilePath;
  document.getElementById("image-preview").style.display = "none";
  document.getElementsByClassName("text4")[0].style.display = "none";
  document.getElementById('fileSize').textContent = "";
}


function handleFileSelect() {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = 'image/*';

  input.onchange = async (event) => {
    const selectedFile = event.target.files[0];

    // Überprüfe, ob die ausgewählte Datei eine GIF-Datei ist
    if (selectedFile && selectedFile.type === 'image/gif') {
      try {
        // Lade die GIF-Datei und erhalte das ImageData-Objekt
        imageDataArray = await loadGifFrameList(URL.createObjectURL(selectedFile));
        
        // Hier kannst du das ImageData-Objekt speichern oder weiterverarbeiten
        console.log("width of imageDataArray[18]: "+imageDataArray[18].width+"  height: "+imageDataArray[18].height);
      } catch (error) {
        console.error('Fehler beim Laden der GIF-Datei:', error);
      }
    } else if (selectedFile) {
      try {
        // Falls es sich um eine einfache Bild-Datei handelt
        const imageData = await loadSingleImage(selectedFile);

        // Erstelle ein ImageData-Array mit dem einzelnen Bild
        imageDataArray = [imageData];
        
        // Hier kannst du das ImageData-Objekt speichern oder weiterverarbeiten
        console.log('ImageData-Array (einfaches Bild):', imageDataArray);
      } catch (error) {
        console.error('Fehler beim Laden des Bildes:', error);
      }
    } else {
      console.log('Bitte wählen Sie eine unterstützte Bild-Datei aus.');
    }
  };

  // Füge das Input-Element zum Dokument hinzu
  document.body.appendChild(input);

  // Triggere den Dateiauswahldialog
  input.click();
}

// Hilfsfunktion zum Laden eines einfachen Bildes
function loadSingleImage(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();

    reader.onload = (event) => {
      const image = new Image();
      image.onload = () => {
        const canvas = document.createElement('canvas');
        const context = canvas.getContext('2d');

        canvas.width = image.width;
        canvas.height = image.height;

        context.drawImage(image, 0, 0);

        const imageData = context.getImageData(0, 0, canvas.width, canvas.height);
        resolve(imageData);
      };

      image.onerror = reject;
      image.src = event.target.result;
    };

    reader.onerror = reject;
    reader.readAsDataURL(file);
  });
}

const loadGifFrameList = async (gifUrl) => {
  const response = await fetch(gifUrl);
  const blob = await response.blob();
  const arrayBuffer = await blob.arrayBuffer();
  const intArray = new Uint8Array(arrayBuffer);

  const reader = new GifReader(intArray);

  const info = reader.frameInfo(0);

  return new Array(reader.numFrames()).fill(0).map((_, k) => {
      const image = new ImageData(info.width, info.height);

      reader.decodeAndBlitFrameRGBA(k, image.data);
      return image;
  });
};

/*
function handleFileSelect() {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = 'image/*';
  
  input.onchange = (event) => {
    const file = event.target.files[0];
    const reader = new FileReader();
    let noGifFrames;
    
    reader.onload = (event) => {
      const img = new Image();
      
      img.onload = () => {
        if (file.type === 'image/gif') {
          img.type = "gif";
          img.file = file;
          console.log("gif read");

        } else {
          img.type = "other";
        }

        window.originalImage = img;         // save as global object

        console.log('Bild vor der Verarbeitung:', img);
        
        var manipulatedPixels = processImage(img, 0);                // scale and display image
        displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);

        // Update the displayed filename
        rdcFileName = document.getElementById('rdc-filename');
        rdcFileName.textContent = replaceFileExtension(file.name);

        console.log ("window.originalImage.noOfFrames: "+window.originalImage.noOfFrames)
        
      };
      
      img.src = event.target.result;
    };
    
    reader.readAsDataURL(file);
  };
  
  input.click();
}
*/

// Function to process the image with aspect ratio preservation
function processImage(img1, frameNo) {
  const scaleCheckbox = document.getElementById('scale');
  let manipulatedPixels;
  frameNo = 18;
  
  console.log("Array length: "+imageDataArray.length);

  selectedFrame = imageDataArray[frameNo];
  // Erstelle ein temporäres Canvas-Element
  const img = document.createElement('canvas');
  const tempCtx = img.getContext('2d');

  // Setze die Breite und Höhe des temporären Canvas auf die des ausgewählten Frames
  img.width = selectedFrame.width;
  img.height = selectedFrame.height;
  console.log("img.width: "+img.width+"  img.height: "+img.height);

  // Zeichne das ImageData-Objekt auf das temporäre Canvas
  tempCtx.putImageData(selectedFrame, 0, 0);
/*
  frameNo=10;

  if (img.type == "gif") {
    const reader = new FileReader();

    console.log("processImage - numberOfFrames: "+img.numberOfFrames);
    console.log('Bild in der Verarbeitung:', img);
        
    reader.readAsArrayBuffer(img.file);

    reader.onload = (event) => {
      const arrayBuffer = event.target.result;
      let myBuffer = new Uint8Array(arrayBuffer);

      const arrayBufferLength = arrayBuffer.byteLength;
      console.log("Länge des arrayBuffer:", arrayBufferLength);
  
      // Use the GifReader library to parse the GIF data
      const gifReader = new GifReader(myBuffer);
      
      img.width = gifReader.width;
      img.height = gifReader.height;
      const GifFrames = gifReader.numFrames();

      console.log("process Image - GifReader.frameNum: "+GifFrames+"   frameNo: "+frameNo+"   xdim: "+img.width+"   ydim: "+img.height);
      window.originalImage.numberOfFrames = GifFrames;

      // Decode the first frame of the GIF
      const pixels = new Uint8Array(img.width * img.height * 4);
      gifReader.decodeAndBlitFrameRGBA(frameNo, pixels);
      
      img.pixels = pixels;

    };


  }
  */

  if (scaleCheckbox.checked) {
    // if checkbox is checked, image will be scaled
    //
    // Create a canvas element.
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
 
    // Set the canvas dimensions to match the loaded image.
//    canvas.width = currentPixelsWidth;
//    canvas.height = currentPixelsHeight;
    canvas.width = img.width;
    canvas.heigth = img.height;

    // Draw the image on the canvas.
    ctx.drawImage(img, 0, 0, currentPixelsWidth, currentPixelsHeight);

    // Get the pixel buffer of the original image.
    const currentImageData = ctx.getImageData(0, 0, currentPixelsWidth, currentPixelsHeight);
    manipulatedPixels = new Uint8ClampedArray(currentImageData.data);
  
  } else {
    // if checkbox is not checked, image will be cropped
    //
    // Create a canvas element.
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');

    // Set the target dimensions for the canvas (e.g., desired width and height)
    const targetWidth = currentPixelsWidth; // Set your desired width
    const targetHeight = currentPixelsHeight; // Set your desired height

    // Calculate the scaling factors for width and height
    const scaleX = targetWidth / img.width;
    const scaleY = targetHeight / img.height;

    // Determine the scaling factor that preserves the aspect ratio
    const scale = Math.max(scaleX, scaleY);

    // Calculate the dimensions after maintaining the aspect ratio
    const scaledWidth = img.width * scale;
    const scaledHeight = img.height * scale;

    // Calculate the offset to center the image on the canvas
    const offsetX = (targetWidth - scaledWidth) / 2;
    const offsetY = (targetHeight - scaledHeight) / 2;

    // Set the canvas dimensions to match the target dimensions
    canvas.width = targetWidth;
    canvas.height = targetHeight;

    // Draw the image on the canvas while maintaining aspect ratio
    ctx.drawImage(img, offsetX, offsetY, scaledWidth, scaledHeight);

    // Get the pixel buffer of the modified image
    const currentImageData = ctx.getImageData(0, 0, targetWidth, targetHeight);
    manipulatedPixels = new Uint8ClampedArray(currentImageData.data);

  }

  // Return the manipulated pixels
  return manipulatedPixels;
}




async function loadImg(filePath) {
  // Fetch the image file from the server.
  const response = await fetch('/downloadfile?filename=' + filePath);
  const blob = await response.blob();

  // Create an image object and load the image data into it.
  const img = new Image();
  img.onload = function() {
    window.originalImage = img; // Speichern Sie das Bild als globales Objekt.

    var manipulatedPixels = processImage(img, 0);                // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);

    // Display the original image filename.
    rdcFileName = document.getElementById('rdc-filename');
    const newRdcFileName = replaceFileExtension(getFileNameFromUrl(filePath));
    rdcFileName.textContent = newRdcFileName;
    console.log("newRdcFileName: "+rdcFileName.textContent);
  };
  img.src = URL.createObjectURL(blob);
};


// Displays the RDC image in an HTML element.
function displayRDC(elementId, pixels, pixelsWidth, pixelsHeight) {
    // Create a canvas element and set its dimensions to match the image size.
    const canvas = document.createElement('canvas');
    canvas.width = pixelsWidth;
    canvas.height = pixelsHeight;
  
    // Draw the image onto the canvas.
    const ctx = canvas.getContext('2d');
    const imageData = ctx.createImageData(canvas.width, canvas.height);
    imageData.data.set(pixels);
    ctx.putImageData(imageData, 0, 0);
  
    // Set the source of the <img> element to the data URL of the canvas.
    const dataUrl = canvas.toDataURL();
    const imgElement = document.getElementById(elementId);
    imgElement.src = dataUrl;
    imgElement.style.display = 'block';
}

const chunkSize = 20000; // Set the chunk size to 100,000 bytes

// create RDC data structure and upload file to server
function saveRDC(fileName) {
    let numberOfFrames;

    if (window.originalImage.type == "gif") {
      numberOfFrames = window.originalImage.numberOfFrames;
    } else {
      numberOfFrames = 1;
    }

    // calculate file size. the first 12 bytes will be 3 unsigned integers (xdim, ydim, no. of images)
    const frameSize = Math.ceil(currentPixelsHeight * 1.5) * currentPixelsWidth;
  
    let rdcData = new Uint8Array(12);

    // Create an array to hold the RDC data
    const fs = frameSize * numberOfFrames + 12;
    console.log(".rdc files size: "+fs);
    rdcData = new Uint8Array(fs);

    // write currentPixelsWidth, currentPixelsHeight, numberOfFrames as unsigned int into the first 12 Bytes
    const dataView = new DataView(rdcData.buffer);
    dataView.setUint32(0, currentPixelsWidth, true);       // "true" means "Little Endian" sequence
    dataView.setUint32(4, currentPixelsHeight, true);
    dataView.setUint32(8, numberOfFrames, true);
    

    // Load to be saved RDC
    for (frameNumber = 0; frameNumber < numberOfFrames; frameNumber++) {
      const pixelsToBeSaved = processImage(window.originalImage, frameNumber);
 
      // Loop through each column of the image
      for (let x = 0; x < currentPixelsWidth; x++) {
        // Loop through each row of the column
        for (let y = 0; y < currentPixelsHeight; y++) {
          // Calculate the index in the currentPixels array
          const pixelIndex = x + (currentPixelsHeight-1-y) * currentPixelsWidth; 
  
          // Get the pixel values (RGB) from currentPixels
          let pixelR = Math.floor(pixelsToBeSaved[pixelIndex*4]/32);
          let pixelG = Math.floor(pixelsToBeSaved[pixelIndex*4+1]/32);
          let pixelB = Math.floor(pixelsToBeSaved[pixelIndex*4+2]/32);
  
          // Calculate the byte index and bit position within the byte
          let y_byte = Math.floor(3*y/2);
          let y_start = (3*y)%2;
          let byte_no = (x * Math.ceil(currentPixelsHeight * 1.5) + y_byte) + frameNumber * frameSize + 12;

          if (y_start==0) {
            rdcData[byte_no] &= 0xF0;
            rdcData[byte_no] |= pixelR;
            pixelG<<=4;
            rdcData[byte_no] &= 0x0F;
            rdcData[byte_no] |= pixelG;
            rdcData[byte_no+1] &= 0xF0;
            rdcData[byte_no+1] |= pixelB;

            let hexString1 = rdcData[byte_no].toString(16);
            if (hexString1.length === 1) {
              hexString1 = '0' + hexString1; 
            }
            let hexString2 = rdcData[byte_no+1].toString(16);
            if (hexString2.length === 1) {
              hexString2 = '0' + hexString2; 
            }
          }
          else {
            pixelR<<=4;
            rdcData[byte_no] &= 0x0F;
            rdcData[byte_no] |= pixelR;
            rdcData[byte_no+1] &= 0xF0;
            rdcData[byte_no+1] |= pixelG;
            pixelB<<=4;
            rdcData[byte_no+1] &= 0x0F;
            rdcData[byte_no+1] |= pixelB;

            let hexString1 = rdcData[byte_no].toString(16);
            if (hexString1.length === 1) {
              hexString1 = '0' + hexString1; 
            }
            let hexString2 = rdcData[byte_no+1].toString(16);
            if (hexString2.length === 1) {
              hexString2 = '0' + hexString2; 
            }

          }
        }
      }
    }
    const totalChunks = Math.ceil(rdcData.length / chunkSize);

    // Start the upload process
    uploadChunksWithRetry(totalChunks, rdcData, fileName);
    
    console.log('File uploaded successfully!');
    // Reload the file list display to reflect the uploaded file
    loadFileList();
};

const uploadChunk = async (chunkNumber, rdcData, fileName) => {
  // Calculate the start and end indices for the current chunk
  const start = chunkNumber * chunkSize;
  const end = Math.min((chunkNumber + 1) * chunkSize, rdcData.length);

  // Get the current chunk of data
  const chunk = rdcData.slice(start, end);  // `chunk.length` wird durch die Länge von `chunk` bestimmt

  // Create a new FormData object for the chunk
  const formData = new FormData();
  formData.append('file', new Blob([chunk], { type: 'application/octet-stream' }), fileName);
  formData.append('chunk', chunkNumber + 1); // Include the chunk number in the FormData

  // Manually set the Content-Length header
  const contentLength = end-start;
  const headers = new Headers();
  headers.append('content_length', contentLength);
  headers.append('chunk', chunkNumber);


  // Determine the URL based on whether it's the first chunk or not
  const url = chunkNumber === 0 ? '/uploadfile' : '/appendfile';

  // Send an HTTP POST request to the server to upload the chunk
  const response = await fetch(`${url}?path=${encodeURIComponent(fileName)}`, {
    method: 'POST',
    body: formData,
    headers: headers, // Add the headers to the fetch request
  });

  return response;
};

const maxRetryAttempts = 3; // Maximale Anzahl von Wiederholungsversuchen

// Function to upload all chunks sequentially
const uploadChunkWithRetry = async (chunkNumber, totalChunks, rdcData, fileName, retryAttempt = 1) => {
  if (retryAttempt > maxRetryAttempts) {
    console.error(`Maximale Anzahl von Wiederholungsversuchen erreicht für Chunk ${chunkNumber + 1}/${totalChunks}!`);
    return;
  }

  try {
    const response = await uploadChunk(chunkNumber, rdcData, fileName);

    if (response.ok) {
      console.log(`Chunk ${chunkNumber + 1}/${totalChunks} uploaded successfully!`);
    } else {
      console.error(`Failed to upload chunk ${chunkNumber + 1}/${totalChunks}! Retrying...`);
      await uploadChunkWithRetry(chunkNumber, totalChunks, rdcData, fileName, retryAttempt + 1); // Wiederholung des Uploads
    }
  } catch (error) {
    console.error(`Error uploading chunk ${chunkNumber + 1}/${totalChunks}: ${error.message}`);
    console.error(`Retrying...`);
    await uploadChunkWithRetry(chunkNumber, totalChunks, rdcData, fileName, retryAttempt + 1); // Wiederholung des Uploads bei einem Fehler
  }
};

// Funktion zum sequentiellen Hochladen aller Chunks mit Wiederholung bei Fehlern
const uploadChunksWithRetry = async (totalChunks, rdcData, fileName) => {
  for (let i = 0; i < totalChunks; i++) {
    await uploadChunkWithRetry(i, totalChunks, rdcData, fileName);
  }
};

function getRDCNames() {
  fetch('/getRDCNames')
    .then((response) => {
      if (response.ok) {
        return response.json();
      } else {
        throw new Error('Fehler bei der Anfrage. Status: ' + response.status);
      }
    })
    .then((data) => {
      if (data.clockface!="") document.getElementById("rdc-clock-face").textContent = getFileNameFromUrl(data.clockface);
      else document.getElementById("rdc-clock-face").textContent = "default";
      document.getElementById("rdc-logo").textContent = getFileNameFromUrl(data.logo);
      document.getElementById("rdc-image").textContent = getFileNameFromUrl(data.image);
    })
    .catch((error) => {
      console.error('Fehler bei der Anfrage:', error);
    });
}

// Extract the file name from a given URL
function getFileNameFromUrl(url) {
  // Splits the URL by "/" and returns the last element of the resulting array, which is the filename
  var parts = url.split("/");
  return parts[parts.length - 1];
}

// Create a canvas element and resize the image to 110x110 pixels.
const canvasPreview = document.createElement('canvas');
canvasPreview.width = 110;
canvasPreview.height = 110;
const ctx = canvasPreview.getContext('2d');

// Get the pixel buffer of the resized image and store it in the global variable.
const imageDataPreview = ctx.getImageData(0, 0, canvasPreview.width, canvasPreview.height);
previewPixels = new Uint8ClampedArray(imageDataPreview.data);
originalPixels = new Uint8ClampedArray(imageDataPreview.data);
currentPixels = new Uint8ClampedArray(imageDataPreview.data);

// Event listener for when the DOM is fully loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('Document loaded.');
    loadFileList();
    getRDCNames();
});


// Event listener for load button
document.getElementById("load-img").onclick = () => {
    if (selectedFilePath !== "") {
        loadImg(selectedFilePath)
    } else {
      handleFileSelect();
    }
    clearSelectedFilePath();
};

// Event listener for "save-rdc" button
document.getElementById("save-rdc").onclick = () => {
    console.log("rdcFileName: " + rdcFileName.textContent);
    console.log("currentPath: " + currentPath);
    console.log("path: " + currentPath + rdcFileName.textContent);    
    saveRDC(rdcFileName.textContent);
};
  
// Event listener for done button
document.getElementById("fm-done").onclick = () => {
  window.location.href = "/configDone";
};

// Event listener for "select clock face" button
document.getElementById("select-clock-face").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectclockface?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-clock-face").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            document.getElementById("rdc-clock-face").textContent = "default";
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};

// Delete a file without confirmation
function deleteFile(filename) {
    // Send an HTTP POST request to the server to delete the specified file
    fetch(`/deletefile?filename=${filename}`)
    .then(response => response.json())
    .then(data => {
        console.log('File deleted:', filename);
        // Reload the file list display to reflect the deleted file
        loadFileList();
    })
    .catch(error => {
        console.error('Error deleting file:', error);
    });
}

// Event listener for "clear clock face" button
document.getElementById("clr-clock-face").onclick = () => {
  
  deleteFile("/variables/clockFacePath.txt");
  document.getElementById("rdc-clock-face").textContent = "default";

};

// Event listener for "select logo" button
document.getElementById("select-logo").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectlogo?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-logo").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};

// Event listener for "select image" button
document.getElementById("select-image").onclick = () => {
  if (selectedFilePath !== "") {
        let rememberSelectedFilePath = selectedFilePath;
        // Send an HTTP POST request to the server to delete the specified file
        fetch(`/selectimage?filename=${selectedFilePath}`)
        .then((response) => {
          if (response.ok) {
            document.getElementById("rdc-image").textContent = getFileNameFromUrl(rememberSelectedFilePath);
            console.log("rememberSelectedFilePath: " + getFileNameFromUrl(rememberSelectedFilePath));
          } else {
            console.log("Fehler bei der Anfrage. Status: " + response.status);
          }
        })
        .catch((error) => {
          // Hier kannst du Fehlerbehandlung durchführen
          console.log("Fehler bei der Anfrage:" + error);
        });
  } else {
    alert("Click and hold to select a .rdc file.");
  }
  clearSelectedFilePath();
};

// Event listener for xdim input field
document.getElementById('xdim').addEventListener('input', function() {
  const newXDim = parseInt(this.value, 10);
  if (!isNaN(newXDim)) {
    currentPixelsWidth = newXDim;
    var manipulatedPixels = processImage(window.originalImage, 0);                // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);


  }
});

// Event listener for ydim input field
document.getElementById('ydim').addEventListener('input', function() {
  const newYDim = parseInt(this.value, 10);
  if (!isNaN(newYDim)) {
    currentPixelsHeight = newYDim;
    var manipulatedPixels = processImage(window.originalImage, 0);                // scale the image
    displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);

  }
});

document.getElementById('scale').addEventListener('change', function () {
  // Hier kannst du den Code platzieren, der bei Änderungen des Checkbox-Status ausgeführt werden soll
  var manipulatedPixels = processImage(window.originalImage, 0);                // scale the image
  displayRDC('image-rdc', manipulatedPixels, currentPixelsWidth, currentPixelsHeight);

});

// Event listener for mousedown event on file list
fileList.onmousedown = (event) => {
  if (event.target !== fileList && !isTouchDevice && !clickHandled) {
    mousedownTime = new Date().getTime();
    clickHandled = true;

    longClickTimeout = setTimeout(() => {
      handleLongClick(event);
    }, 500);
  }
};

// Event listener for mouseup event on file list
fileList.onmouseup = (event) => {
  if (event.target !== fileList && !isTouchDevice && clickHandled) {
    clearTimeout(longClickTimeout);

    mouseupTime = new Date().getTime();
    const timeDifference = mouseupTime - mousedownTime;

    if (timeDifference < 500) {
      handleShortClick(event);
    }

    // Wartezeit von 500 ms, bevor weitere Klicks akzeptiert werden
    setTimeout(() => {
      clickHandled = false;
    }, 500);
  }
};

// Event listener for touchstart event on file list
fileList.addEventListener('touchstart', (event) => {
  if (event.target !== fileList && !clickHandled) {
    isTouchDevice = true;
    touchStartX = event.touches[0].clientX;
    touchStartY = event.touches[0].clientY;
    touchMoved = false;
    clickHandled = true;

    mousedownTime = new Date().getTime();

    longClickTimeout = setTimeout(() => {
      handleLongClick(event);
    }, 500);
  }
});

// Event listener for touchmove event on file list
fileList.addEventListener('touchmove', (event) => {
  if (event.target !== fileList) {
    const touchEndX = event.touches[0].clientX;
    const touchEndY = event.touches[0].clientY;
    const touchDistance = Math.sqrt(
      Math.pow(touchEndX - touchStartX, 2) + Math.pow(touchEndY - touchStartY, 2)
    );

    if (touchDistance > 10) {
      touchMoved = true;
      clearTimeout(longClickTimeout);
    }
  }
});

// Event listener for touchend event on file list
fileList.addEventListener('touchend', (event) => {
  if (event.target !== fileList && clickHandled) {
    clearTimeout(longClickTimeout);

    mouseupTime = new Date().getTime();
    const timeDifference = mouseupTime - mousedownTime;

    if (!touchMoved && timeDifference < 500) {
      handleShortClick(event);
    }

    // Wartezeit von 500 ms, bevor weitere Klicks akzeptiert werden
    setTimeout(() => {
      clickHandled = false;
    }, 500);
  }

  touchMoved = false;
});

// Check if it is a touch-enabled device
if ('ontouchstart' in window || navigator.msMaxTouchPoints) {
    scrollDirection = -1; // Reverse scroll direction for touch-enabled devices
}  
  
// Handle short click event on file item
function handleShortClick(event) {
  console.log('Short click event triggered.'); // Überprüfen Sie, ob diese Zeile im Konsolenprotokoll erscheint.

  fileItems.forEach(item => {
    item.classList.remove("selected");
  });

  event.target.classList.add("selected");
  const fileName = event.target.textContent;
  
  if (fileName.indexOf('.') === -1) {
    currentPath += fileName + "/";
    console.log("Current path: " + currentPath);
    loadFileList();
  }

  previewFile(currentPath + fileName);
}

// Handle long click event on file item
function handleLongClick(event) {
  selectedFile = event.target.textContent;
  if (selectedFile.indexOf("..") !== -1) {
    // Do nothing
  } else if (selectedFile.indexOf(".") !== -1) {
    selectedFilePath = currentPath + selectedFile;
    document.getElementById("text-selected-file").value = "Selected file: " + selectedFilePath;
    previewFile(selectedFilePath);
  } else {
    selectedFilePath = currentPath + selectedFile +"/";
    document.getElementById("text-selected-file").value = "Selected folder: " + selectedFilePath;
  }
}

