//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include <math.h> 

#include "RayTrace.h"
#include "master.h"

void masterMain(ConfigData* data)
{
  //Depending on the partitioning scheme, different things will happen.
  //You should have a different function for each of the required 
  //schemes that returns some values that you need to handle.
  
  //Allocate space for the image on the master.
  float* pixels = new float[3 * data->width * data->height];
  
  //Execution time will be defined as how long it takes
  //for the given function to execute based on partitioning
  //type.
  double renderTime = 0.0, startTime, stopTime;
  
  //Fill in the MPI related data
  int rank, numtasks;
  MPI_Comm_rank(MPI_COMM_WORLD, & data->mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, & data->mpi_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  MPI_Status status;

	//Add the required partitioning methods here in the case statement.
	//You do not need to handle all cases; the default will catch any
	//statements that are not specified. This switch/case statement is the
	//only place that you should be adding code in this function. Make sure
	//that you update the header files with the new functions that will be
	//called.
	//It is suggested that you use the same parameters to your functions as shown
	//in the sequential example below.
  
  double comp_start, comp_stop, comp_time;
  
  switch (data->partitioningMode)
  {
    case PART_MODE_NONE:
    {
      startTime = MPI_Wtime();
      //Call the function that will handle this.
      masterSequential(data, pixels);
      stopTime = MPI_Wtime();
      break;
    }
    case PART_MODE_STATIC_STRIPS_VERTICAL:
    {
      startTime = MPI_Wtime();
      comp_start = MPI_Wtime();
      
      int add = (data->width) % numtasks;
      int realWidth = (data->width)/numtasks;
      if(rank < add)
      {
        realWidth += 1;
      }
      std::cout << "realWidth:   " << realWidth << std::endl;
      
      for (int i = 0; i < data->height; ++i) 
      {
        for (int j = 0; j < realWidth; ++j) 
        {
          //Calculate the index into the array.
          int row = i;
          // int column = (rank * data->width/numtasks) + temp + j;
          // This should be just j for rank = 0
          int column = j;
          
          int baseIndex = 3*(data->height*row+ column);
          //Call the function to shade the pixel.
          shadePixel(&(pixels[baseIndex]), row, column, data);
        }
      }
      comp_stop = MPI_Wtime();
      comp_time = comp_stop - comp_start;
      double compTimes = comp_time;
      
      //Receive data from slaves
      int pixelIndex = 0;
      for(int h = 1; h < numtasks; h++)
      {
        int incomingWidth = data->width/numtasks;
        if(h < add)
        {
          incomingWidth += 1;
        }
        int incomeTemp = (3 * incomingWidth * data->height)+1;
        float* incomingData = new float[incomeTemp];
        double incomingCompTime = 0;
        std::cout << "receive rank:" << h << " incomeTemp: " << incomeTemp<< std::endl;
        MPI_Recv(&incomingData, incomeTemp, MPI_FLOAT, h, 4, MPI_COMM_WORLD, &status);
        MPI_Recv(&incomingCompTime, 1, MPI_DOUBLE, h, 2, MPI_COMM_WORLD, &status);
        std::cout << "received rank:" << h << std::endl;
        compTimes += incomingCompTime;
        
        std::cout << " pixelIndex:  "<< pixelIndex<< std::endl;
        for(int w = 0; w < (3*incomingWidth*data->height); w++)
        {
          pixels[pixelIndex+w] = incomingData[w];
        }
        
        pixelIndex += 3 * incomingWidth * data->height;
      }
      std::cout << "rank:" << rank << " finished receiving"<< std::endl;
      
      stopTime = MPI_Wtime();
      renderTime = stopTime - startTime;
      
      double communicationTime = renderTime-compTimes;
      
      //Print the times and the c-to-c ratio
      //This section of printing, IN THIS ORDER, needs to be included in all of the
      //functions that you write at the end of the function.
      std::cout << "Total Computation Time: " << compTimes << " seconds" << std::endl;
      std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
      double c2cRatio = communicationTime / compTimes;
      std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
      
      break;
    }
    case PART_MODE_STATIC_CYCLES_VERTICAL:
    {
      comp_start = MPI_Wtime();
      
      int numCycles = data->height / data->cycleSize;
      for(int n = 1; n < numCycles; n++)
      {
        if(n % numtasks == rank)
        {
          for(int x = 0; x < data->cycleSize; x++)
          {
            for(int y = 0; y < data->height; y++)
            {
              int row = y;
              int column = n*data->cycleSize + x;
              int baseIndex =  3*(row * data->width + column);
              shadePixel(&(pixels[baseIndex]), row, column, data);
            }
          }
        }
      }
      //master will do the extra columns
      if(data->width % cycleSize > 0)
      {
        for(int i = 0; i < data->width % numtasks; i++)
        {
          for(int y = 0; y < data->height; y++)
          {
            int row = y;
            int column = numCycles*data->cycleSize + i;
            int baseIndex =  3*(row * data->width + column);
            shadePixel(&(pixels[baseIndex]), row, column, data);
          }
        }
      }
      comp_stop = MPI_Wtime();
      comp_time = comp_stop - comp_start;
      double compTimes = comp_time;
      
      for(int h = 1; h < numtasks; h++)
      {
        int incomeTemp = ;
        MPI_Recv(&incomingData, incomeTemp, MPI_FLOAT, source, 4, MPI_COMM_WORLD, &status);
        MPI_Recv(&incomingCompTime, 1, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &status);
        compTimes += incomingCompTime;
      }
      
      break;
    }
    case PART_MODE_STATIC_BLOCKS:
    {
      break;
    }
    case PART_MODE_STATIC_CYCLES_HORIZONTAL:
    {
      break;
    }
    case PART_MODE_DYNAMIC:
    {
      break;
    }
    case PART_MODE_STATIC_STRIPS_HORIZONTAL:
    {
      std::cout << "This mode (" << data->partitioningMode;
      std::cout << ") is not currently implemented." << std::endl;
      break;
    }
    default:
    {
      std::cout << "This mode (" << data->partitioningMode;
      std::cout << ") is not currently implemented." << std::endl;
      break;
    }
  }

  renderTime = stopTime - startTime;
  std::cout << "Execution Time: " << renderTime << " seconds" << std::endl << std::endl;

  //After this gets done, save the image.
  std::cout << "Image will be save to: ";
  std::string file = generateFileName(data);
  std::cout << file << std::endl;
  savePixels(file, pixels, data);

  //Delete the pixel data.
  delete[] pixels; 
}

void masterSequential(ConfigData* data, float* pixels)
{
  //Start the computation time timer.
  double computationStart = MPI_Wtime();

  //Render the scene.
  for( int i = 0; i < data->height; ++i )
  {
    for( int j = 0; j < data->width; ++j )
    {
      int row = i;
      int column = j;

      //Calculate the index into the array.
      int baseIndex = 3 * ( row * data->width + column );

      //Call the function to shade the pixel.
      shadePixel(&(pixels[baseIndex]),row,j,data);
    }
  }

  //Stop the comp. timer
  double computationStop = MPI_Wtime();
  double computationTime = computationStop - computationStart;

  //After receiving from all processes, the communication time will
  //be obtained.
  double communicationTime = 0.0;

  //Print the times and the c-to-c ratio
  //This section of printing, IN THIS ORDER, needs to be included in all of the
  //functions that you write at the end of the function.
  std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
  std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
  double c2cRatio = communicationTime / computationTime;
  std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}