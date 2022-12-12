#include <Arduino.h>
#include <ArduinoOTA.h>
// #include <SPI.h>
// #include <WiFi.h>
// #include <WiFiClient.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "SPIFFS.h"
#include <myfunction.h>
#include "html.h"



const byte MLX90640_address = 0x33;  // Default 7-bit unshifted address of the MLX90640
paramsMLX90640 mlx90640;
float mlx90640To[768];
#define TA_SHIFT 8  // Default shift for MLX90640 in open air
#define I2C_SCL 14	// pb avec 12 et 13 sur ESP32 CAM
#define I2C_SDA 2

// You can use any (4 or) 5 pins
#define sclk 18
#define mosi 23
#define cs 17
#define rst 5
#define dc 16

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

// the colors we will be using
#include <colors.h>

AsyncWebServer server(80);

// Enter your SSID and PASSWORD
// const char *ssid = "ESP";
// const char *password = "camera";

// float MaxTemp = 0;
// float MinTemp = 0;
// float CenterTemp = 0;

// String getCenterTemp() {
// 	extern float CenterTemp;
// 	return String(CenterTemp);
// }
// String getMaxTemp() {
// 	extern float MaxTemp;
// 	return String(MaxTemp);
// }
// String getMinTemp() {
// 	extern float MinTemp;
// 	return String(MinTemp);
// }


// Replaces placeholder with values
// String processor(const String &var) {
// 	if (var == "TEMPERATURE") {
// 		return getCenterTemp();
// 	}
// 	if (var == "TEMPMAX") {
// 		return getMaxTemp();
// 	}
// 	if (var == "TEMPMIN") {
// 		return getMinTemp();
// 	}

// 	return String();
// }

// Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected() {
	Wire.beginTransmission((uint8_t)MLX90640_address);
	if (Wire.endTransmission() != 0)
		return (false);  // Sensor did not ACK
	return (true);
}

boolean initMLX90640(){
	Wire.begin(I2C_SDA, I2C_SCL, 400000); // Increase I2C clock speed to 400kHz

	if (isConnected() )
		DEBUGLOG("MLX90640 online !\n");
	else
		DEBUGLOG("MLX90640 not detected at default I2C address. Please check wiring.\n");

	// Get device parameters - We only have to do this once
	int status = 0;
	uint16_t eeMLX90640[832];
	status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
	if (status)
		DEBUGLOG("MLX90640 Failed to load system parameters\n");
	else
		DEBUGLOG("MLX90640 system parameters loaded\n");

	status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
	if (status)
		DEBUGLOG("MLX90640 Parameter extraction failed\n");
	else
		DEBUGLOG("MLX90640 Parameter extracted\n");

	int SetRefreshRate = MLX90640_SetRefreshRate(MLX90640_address, 0x03);
	// int SetInterleavedMode = MLX90640_SetInterleavedMode(MLX90640_address);
	int SetChessMode = MLX90640_SetChessMode(MLX90640_address);

	return true;
}

void ThermalImageToWeb(float mlx90640To[], float MinTemp = 0, float MaxTemp = 40) {
	// --- SAVE BMP FILE --- //
	uint8_t colorIndex = 0;
	uint16_t color = 0;
	unsigned int headers[13];
	int extrabytes;
	int paddedsize;
	int x = 0;
	int y = 0;
	int n = 0;
	int red = 0;
	int green = 0;
	int blue = 0;

	int WIDTH = 32;
	int HEIGHT = 24;

	extrabytes = 4 - ((WIDTH * 3) % 4);  // How many bytes of padding to add to each
										 // horizontal line - the size of which must
										 // be a multiple of 4 bytes.
	if (extrabytes == 4)
		extrabytes = 0;

	paddedsize = ((WIDTH * 3) + extrabytes) * HEIGHT;

	// Headers...
	// Note that the "BM" identifier in bytes 0 and 1 is NOT included in these "headers".

	headers[0] = paddedsize + 54;  // bfSize (whole file size)
	headers[1] = 0;                // bfReserved (both)
	headers[2] = 54;               // bfOffbits
	headers[3] = 40;               // biSize
	headers[4] = WIDTH;            // biWidth
	headers[5] = HEIGHT;           // biHeight

	// Would have biPlanes and biBitCount in position 6, but they're shorts.
	// It's easier to write them out separately (see below) than pretend
	// they're a single int, especially with endian issues...

	headers[7] = 0;           // biCompression
	headers[8] = paddedsize;  // biSizeImage
	headers[9] = 0;           // biXPelsPerMeter
	headers[10] = 0;          // biYPelsPerMeter
	headers[11] = 0;          // biClrUsed
	headers[12] = 0;          // biClrImportant

	// outfile = fopen(filename, "wb");

	File file = SPIFFS.open("/thermal.bmp", "wb");
	if (!file) {
		Serial.println("There was an error opening the file for writing");
		// return;
	} else {
		// Headers begin...
		// When printing ints and shorts, we write out 1 character at a time to avoid endian issues.

		file.print("BM");

		for (n = 0; n <= 5; n++) {
			file.printf("%c", headers[n] & 0x000000FF);
			file.printf("%c", (headers[n] & 0x0000FF00) >> 8);
			file.printf("%c", (headers[n] & 0x00FF0000) >> 16);
			file.printf("%c", (headers[n] & (unsigned int)0xFF000000) >> 24);
		}

		// These next 4 characters are for the biPlanes and biBitCount fields.

		file.printf("%c", 1);
		file.printf("%c", 0);
		file.printf("%c", 24);
		file.printf("%c", 0);

		for (n = 7; n <= 12; n++) {
			file.printf("%c", headers[n] & 0x000000FF);
			file.printf("%c", (headers[n] & 0x0000FF00) >> 8);
			file.printf("%c", (headers[n] & 0x00FF0000) >> 16);
			file.printf("%c", (headers[n] & (unsigned int)0xFF000000) >> 24);
		}

		// Headers done, now write the data...

		for (y = HEIGHT - 1; y >= 0; y--)  // BMP image format is written from bottom to top...
		{
			for (x = 0; x <= WIDTH - 1; x++) {
				// --- Read ColorIndex corresponding to Pixel Temperature --- //
				colorIndex = map(mlx90640To[x + (32 * y)], MinTemp - 5.0, MaxTemp + 5.0, 0, 255);
				colorIndex = constrain(colorIndex, 0, 255);
				color = camColors[colorIndex];

				// --- Converts 4 Digits HEX to RGB565 --- //
				// uint8_t r = ((color >> 11) & 0x1F);
				// uint8_t g = ((color >> 5) & 0x3F);
				// uint8_t b = (color & 0x1F);

				// --- Converts 4 Digits HEX to RGB565 -> RGB888 --- //
				red = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
				green = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
				blue = (((color & 0x1F) * 527) + 23) >> 6;

				// --- RGB range from 0 to 255 --- //
				if (red > 255) red = 255;
				if (red < 0) red = 0;
				if (green > 255) green = 255;
				if (green < 0) green = 0;
				if (blue > 255) blue = 255;
				if (blue < 0) blue = 0;

				// Also, it's written in (b,g,r) format...

				file.printf("%c", blue);
				file.printf("%c", green);
				file.printf("%c", red);
			}
			if (extrabytes)  // See above - BMP lines must be of lengths divisible by 4.
			{
				for (n = 1; n <= extrabytes; n++) {
					file.printf("%c", 0);
				}
			}
		}

		file.close();
		DEBUGLOG("File Closed\n");
	}  // --- END SAVING BMP FILE --- //
}

// SETUP
//==========================================================================

void setup() {
	DEBUGINIT();
	mySmartConfig();

	// ESP32 As access point
	// WiFi.mode(WIFI_AP);  // Access Point mode
	// WiFi.softAP(ssid, password);

	initMLX90640();


	if (SPIFFS.begin(true))
		DEBUGLOG("SPIFFS mounted !\n");
	else
		DEBUGLOG("An Error has occurred while mounting SPIFFS\n");


	// --- Part WebServer ESP --- //

	// Route for root / web page
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		// request->send_P(200, "text/html", index_html, processor);
		request->send_P(200, "text/html", index_html);
	});
	// server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send_P(200, "text/plain", getCenterTemp().c_str());
	// });
	// server.on("/tempmax", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send_P(200, "text/plain", getMaxTemp().c_str());
	// });
	// server.on("/tempmin", HTTP_GET, [](AsyncWebServerRequest *request) {
	// 	request->send_P(200, "text/plain", getMinTemp().c_str());
	// });
	server.on("/thermal", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/thermal.bmp", "image/bmp", false);
	});

	server.begin();  // Start server

	ArduinoOTA.begin();
	DEBUGLOG("Setup Done\n");
}

// LOOP
//===========================================================================

void loop() {
	ArduinoOTA.handle();

	// Read Thermal Image from MLX90640
	for (byte x = 0; x < 2; x++)  // Read both subpages
	{
		uint16_t mlx90640Frame[834];
		int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
		if (status < 0)
			DEBUGLOG("GetFrame Error: %d\n",status);

		float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
		float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

		float tr = Ta - TA_SHIFT;  // Reflected temperature based on the sensor ambient temperature
		float emissivity = 0.95;

		MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
	}

	// --- START of Calculate Chess Mode --- //
	// Calculate difference between Subpages (chess-mode)
	    int pa = 0;
		int niepa = 0;
		float sumpa = 0;
		float sumniepa = 0;

		int w = 32;
		int h = 24;

		for(int i=0; i<h; i++) {
		  for(int j=0; j<w; j++) {
			if((i+j)%2 == 0){
			  sumpa = mlx90640To[j+(w*i)];
			  pa++;
			}else{
			  sumniepa = mlx90640To[j+(w*i)];
			  niepa++;
			}
		  }
		}

		sumpa = sumpa / (float)pa;
		sumniepa = sumniepa / (float)niepa;
		float diff = sumpa - sumniepa;          // Difference between even and odd

		if(diff < 0.0){
		  for(int i=0; i<h; i++) {
			for(int j=0; j<w; j++) {
			  if((i+j)%2 == 0){
				mlx90640To[j+(w*i)] += abs(diff);
			  }else{
				//mlx90640To[j+(w*i)] += abs(diff);
			  }
			}
		  }
		}else{
		   for(int i=0; i<h; i++) {
			for(int j=0; j<w; j++) {
			  if((i+j)%2 == 0){
				//mlx90640To[j+(w*i)] += abs(diff);
			  }else{
				mlx90640To[j+(w*i)] += abs(diff);
			  }
			}
		  }
		}       
	// --- END of Calculate Chess Mode --- //

	// CenterTemp = (mlx90640To[165] + mlx90640To[180] + mlx90640To[176] + mlx90640To[192]) / 4.0;  // Temp in Center - based on 4 pixels

	// MaxTemp = mlx90640To[0];  // Get first data to find Max and Min Temperature
	// MinTemp = mlx90640To[0];

	// for (int x = 0; x < 768; x++)  // Find Maximum and Minimum Temperature
	// {
	// 	if (mlx90640To[x] > MaxTemp) {
	// 		MaxTemp = mlx90640To[x];
	// 	}
	// 	if (mlx90640To[x] < MinTemp) {
	// 		MinTemp = mlx90640To[x];
	// 	}
	// }

	// ThermalImageToWeb(mlx90640To, MinTemp, MaxTemp);
	ThermalImageToWeb(mlx90640To);
	delay(100);
}
