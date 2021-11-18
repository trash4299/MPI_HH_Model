//This file contains the code that the slave process will execute.

#include <iostream>
#include <mpi.h>
#include <math.h> 

#include "RayTrace.h"
#include "slave.h"

void slaveMain(ConfigData * data) 
{
  //Depending on the partitioning scheme, different things will happen.
  //You should have a different function for each of the required 
  //schemes that returns some values that you need to handle.
  double comp_start, comp_stop, comp_time;
  int rank, numtasks;
  int destination = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
  
  float* pixels = new float[3 * data->width * data->height + 1];
  
  switch (data->partitioningMode) 
  {
    case PART_MODE_NONE:
      //The slave will do nothing since this means sequential operation.
      break;
    case PART_MODE_STATIC_STRIPS_VERTICAL:
    {
      comp_start = MPI_Wtime();
      int add = data->width % numtasks;
      int realWidth = data->width/numtasks;
      int temp = 0;
      if(add > 0) 
      {
        if(rank <= add)
        {
          temp = rank;
        }
        else
        {
          temp = add;
        }
        if(rank < add)
        {
          realWidth += 1;
        }
      }
      
      for (int i = 0; i < data->height; ++i) 
      {
        for (int j = 0; j < realWidth; ++j) 
        {
          //Calculate the index into the array.
          int row = i;
          int column = (rank * ((int)data->width/numtasks)) + temp + j;
          
          int baseIndex = 3*(row * data->width + column);
          
          //Call the function to shade the pixel.
          shadePixel(&(pixels[baseIndex]), row, column, data);
        }
      }
      comp_stop = MPI_Wtime();
      comp_time = comp_stop - comp_start;
      
      int typeIndex = 3 * data->height * ((rank * ((int) data->width/numtasks))+temp);
      int count = 1; 
      int blklens[count] = {3*realWidth*data->height}; 
      int displs[count] = {typeIndex};
      std::cout << "rank: "<<rank<<"  blklens:"<< blklens[0]<<" "<<blklens[1]<<"  displs:"<<displs[0]<<" "<<displs[1]<<std::endl;
      MPI_Datatype newtype;
      MPI_Type_indexed(count, blklens, displs, MPI_FLOAT, &newtype);
      MPI_Type_commit(&newtype);
      
      MPI_Send(&pixels, 1, newtype,destination, 4, MPI_COMM_WORLD);
      std::cout << "rank:" << rank << " finished sending pixels"<< std::endl;
      MPI_Send(&comp_time, 1, MPI_DOUBLE,destination, 2, MPI_COMM_WORLD);
      std::cout << "rank:" << rank << " finished sending compTime"<< std::endl;
      
      break;
    }
    case PART_MODE_STATIC_CYCLES_VERTICAL:
    {
      comp_start = MPI_Wtime();
      
      int numCycles = data->height / data->cycleSize;
      for(int n = 1; n < numCycles; n++)
      {
        if(numCycles%numtasks == rank)
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
      
      comp_stop = MPI_Wtime();
      comp_time = comp_stop - comp_start;
      
      // Add comp time to array
      int pixelsWidth = (3 * data->width * data->height);
      pixels[pixelsWidth] = (float) comp_time;
      
      
      
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
      std::cout << "This mode (" << data -> partitioningMode;
      std::cout << ") is not currently implemented." << std::endl;
      break;
    }
  }
  //Delete the pixel data.
  delete[] pixels; 
}