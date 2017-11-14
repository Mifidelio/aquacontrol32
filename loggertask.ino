void loggerTask ( void * pvParameters )
{
  const uint64_t loggerTaskdelayTime  = ( 1000 * 60 ) / portTICK_PERIOD_MS;
  const uint64_t SPI_MutexMaxWaitTime =           100 / portTICK_PERIOD_MS;

  Serial.println( "Data logger task init." );

  if ( !numberOfFoundSensors )
  {
    Serial.println( "No dallas sensors. Exiting task." );
    vTaskDelete( NULL );
  }

  if ( !xSemaphoreTake( x_SPI_Mutex, SPI_MutexMaxWaitTime ) )
  {
    Serial.println( "Could not find sd card. SPI bus not available" );
    vTaskDelete( NULL );
  }

  if ( !SD.begin( SPI_SD_CS_PIN, SPI, 2000000 ) )
  {
    Serial.println( "No sd available. No logging. Exiting task." );
    xSemaphoreGive( x_SPI_Mutex );
    vTaskDelete( NULL );
  }

  sdcardPresent = true;
  uint64_t cardSize = SD.cardSize() / ( 1024 * 1024 );
  Serial.printf( "SD Card Size: %lluMB\n", cardSize );
  xSemaphoreGive( x_SPI_Mutex );

  while (1)
  {
    if ( xSemaphoreTake( x_SPI_Mutex, SPI_MutexMaxWaitTime ) )
    {
      char content[40];
      uint8_t charCount = 0;

      charCount += snprintf( content, sizeof( content ), "%i,", time( NULL ) );

      if ( numberOfFoundSensors )
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, "%3.2f", sensor[ 0 ].temp / 16.0 );

        for  ( uint8_t sensorNumber = 1; sensorNumber < numberOfFoundSensors; sensorNumber++ )
        {
          charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%3.2f", sensor[ sensorNumber ].temp / 16.0 );
        }
      }
      else
      {
        charCount += snprintf( content + charCount, sizeof( content ) - charCount, ",%s", "No sensors." );
      }

      struct tm timeinfo;
      getLocalTime( &timeinfo );

      char fileName[17];
      strftime( fileName , sizeof( fileName ), "/%F.log", &timeinfo );

      if ( !writelnFile( SD, fileName, content ) )
      {
        Serial.println("Failed to open file for appending");
      }
      else
      {
        Serial.printf( "Written '%s' to '%s'.\n", content, fileName );
      }
      xSemaphoreGive( x_SPI_Mutex );
    }
    else
    {
      Serial.println( "Skipped saving log file entry. SPI bus not available." );
    }
    vTaskDelay( loggerTaskdelayTime );
  }
}

bool writelnFile( fs::FS &fs, const char * path, const char * message )
{
  File file = fs.open( path, FILE_APPEND );
  if ( !file )
  {
    Serial.println("Failed to open file for writing");
    return false;
  }
  if ( !file.println( message ) )
  {
    Serial.println("File write failed");
    file.close();
    return false;
  }
  file.close();
  return true;
}
/*
  void listDir( fs::FS &fs, const char * dirname, uint8_t levels )
  {
    Serial.printf( "Listing directory: %s\n", dirname );

    File root = fs.open( dirname );
    if( !root ){
        Serial.println( "Failed to open directory" );
        return;
    }
    if( !root.isDirectory() ){
        Serial.println( "Not a directory" );
        return;
    }

    File file = root.openNextFile();
    while(file)
    {
        if(file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
  }
*/