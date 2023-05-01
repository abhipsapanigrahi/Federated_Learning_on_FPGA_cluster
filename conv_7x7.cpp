///////////////////////////////////////////////////////////////////////////////
// Author:      Virat Agarwal
// Course:      ECE8893 - Parallel Programming for FPGAs
// Filename:    conv_7x7.cpp
// Description: Implement a functionally-correct synthesizable 7x7 convolution 
//              for a single tile block without any optimizations
///////////////////////////////////////////////////////////////////////////////
#include "utils.h"
#include "hls_math.h"
#include <cmath>
#include <iostream>

using namespace std;

/*void conv_7x7(
    fm_t Y_buf[OUT_BUF_DEPTH][OUT_BUF_HEIGHT][OUT_BUF_WIDTH], 
    fm_t X_buf[IN_BUF_DEPTH][IN_BUF_HEIGHT][IN_BUF_WIDTH],
    wt_t W_buf[OUT_BUF_DEPTH][IN_BUF_DEPTH][KERNEL_HEIGHT][KERNEL_WIDTH],
    wt_t B_buf[OUT_BUF_DEPTH]
)*/
void conv_5x5(
    fm_t Y_buf[OUT_BUF_DEPTH][OUT_BUF_HEIGHT][OUT_BUF_WIDTH],
    fm_t X_buf[IN_BUF_DEPTH][IN_BUF_HEIGHT][IN_BUF_WIDTH],
    wt_t W_buf[OUT_BUF_DEPTH][IN_BUF_DEPTH][KERNEL_HEIGHT][KERNEL_WIDTH]
)
{
    //---------------------------------------------------------------------------
    // Part B: Implement a trivial functionally-correct single-tile convolution.
    //
    //         This should have an overall latency in the order of 22-23 seconds.
    //
    //         If it's worse than trivial, it may be worth fixing this first.
    //         Otherwise, achieving the target latency with a worse-than-trivial
    //         baseline may be difficult!
    //
    // TODO: Your code for Part B goes here. 
    //---------------------------------------------------------------------------

    // Loading the iimage locally
    for (int feature = 0; feature < OUT_BUF_DEPTH; feature++) {
        //for (int i = 0; i < TILE_HEIGHT; i += 2) {
        for (int i = 0; i < TILE_HEIGHT; i++) {
            //for (int j = 0; j < TILE_WIDTH; j += 2) {
            for (int j = 0; j < TILE_WIDTH; j++) {
                fm_t tmp = 0;
                for (int k = 0; k < IN_BUF_DEPTH; k++) {
                    for (int weight_row = 0; weight_row < KERNEL_HEIGHT; weight_row++) {
                        int row = i + weight_row;
                        for (int weight_column = 0; weight_column < KERNEL_WIDTH; weight_column++) {
                            int column = j + weight_column;
                            tmp += W_buf[feature][k][weight_row][weight_column] * X_buf[k][row][column];
                        }
                    }
                }
                //int tmp_row = i / 2;
                //int tmp_column = j / 2;
                //Y_buf[feature][tmp_row][tmp_column] = B_buf[feature] + tmp;

                // ReLU in-place
                Y_buf[feature][i][j] = (tmp > 0) ? (tmp) : (fm_t) 0;
//		cout<< Y_buf[feature][i][j]<<std::endl;
            }
        }
    }
}

void max_pool(
    fm_t conv_in_buf[OUT_BUF_DEPTH][OUT_BUF_HEIGHT][OUT_BUF_WIDTH],
    fm_t max_pool_out_buf[OUT_MAX_POOL_BUF_DEPTH][OUT_MAX_POOL_BUF_HEIGHT][OUT_MAX_POOL_BUF_WIDTH],
    int dim
)
{
    // Loading the image locally
    for (int depth = 0; depth < OUT_MAX_POOL_BUF_DEPTH; depth++) {
        for (int i = 0; i < OUT_MAX_POOL_BUF_HEIGHT; i++) {
            for (int j = 0; j < OUT_MAX_POOL_BUF_WIDTH; j++) {
                fm_t max = -1;
                for (int ii = 0; ii < dim; ii++) {
                    int row = i * dim + ii;
                    for (int jj = 0; jj < dim; jj++) {
                        int col = j * dim + jj;
                        max = (conv_in_buf[depth][row][col] > max) ? conv_in_buf[depth][row][col] : max;
                    }
                }
                max_pool_out_buf[depth][i][j] = max;
		//cout<<max_pool_out_buf[depth][i][j]<<endl;
            }
        }
    }
}

void quarter_drop(
    fm_t max_pool_out_buf[OUT_MAX_POOL_BUF_DEPTH][OUT_MAX_POOL_BUF_HEIGHT][OUT_MAX_POOL_BUF_WIDTH]
)
{

    // Loading the image locally
    for (int depth = 0; depth < OUT_MAX_POOL_BUF_DEPTH; depth++) {
        for (int i = 0; i < OUT_MAX_POOL_BUF_HEIGHT; i++) {
            for (int j = 0; j < OUT_MAX_POOL_BUF_WIDTH; j++) {
                int value = depth * OUT_MAX_POOL_BUF_HEIGHT * OUT_MAX_POOL_BUF_WIDTH + i * OUT_MAX_POOL_BUF_WIDTH + j;	
                unsigned int rand_no = pseudo_random(31, value);
                if (!(rand_no % 4)) {
                    max_pool_out_buf[depth][i][j] = 0;
                }
            }
        }
    }
}

void linear_layer(
    fm_t linear_input[OUT_MAX_POOL_FM_DEPTH * OUT_MAX_POOL_FM_HEIGHT * OUT_MAX_POOL_FM_WIDTH],
    wt_t linear_weights[IN_LINEAR_LENGTH][OUT_LINEAR_LENGTH],
    fm_t inferred_feature_map[OUT_LINEAR_LENGTH]
)
{
    for (int i = 0; i < OUT_LINEAR_LENGTH; i++) {
        fm_t temp = 0;
        for (int j = 0; j < IN_LINEAR_LENGTH; j++) {
            temp += linear_input[j] * linear_weights[j][i];
        }
        //std:cout << temp << std::endl;
        inferred_feature_map[i] = (temp > 0) ? temp : (fm_t) 0;
    }
}

void softmax(
    fm_t inferred_feature_map[OUT_LINEAR_LENGTH],
    fm_t softmax_output[OUT_LINEAR_LENGTH]
)
{
    fm_tsum sum = 0;
    for (int i = 0; i < OUT_LINEAR_LENGTH; i++) {
        std::cout << "Inferred value : " << inferred_feature_map[i] << std::endl;
#ifdef CSIM_DEBUG
        sum += (fm_t) exp(inferred_feature_map[i]);
#else
        //sum += (fm_t) hls::exp(inferred_feature_map[i]);
        sum += hls::exp((ap_fixed<16,5>) inferred_feature_map[i]);
#endif 
        std::cout << "Exponent sum for " << i << " is " << sum << std::endl;
    }
    for (int i = 0; i < OUT_LINEAR_LENGTH; i++) {
#ifdef  CSIM_DEBUG
        softmax_output[i] = (fm_t) exp(inferred_feature_map[i]) / sum;
#else
        //softmax_output[i] = (fm_t) hls::exp(inferred_feature_map[i]) / sum;
        softmax_output[i] = hls::exp((ap_fixed<16, 5>) inferred_feature_map[i]) / sum;
#endif
        std::cout << "Probability of " << i << " is " << softmax_output[i] << std::endl;
    }
}

void backprop(
    fm_t linear_input[OUT_MAX_POOL_FM_DEPTH * OUT_MAX_POOL_FM_HEIGHT * OUT_MAX_POOL_FM_WIDTH],
    wt_t linear_weights[IN_LINEAR_LENGTH][OUT_LINEAR_LENGTH],
    fm_t output_feature_map[OUT_LINEAR_LENGTH], //This is output of softmax
    fm_t target_output[OUT_LINEAR_LENGTH]
)
{
    fm_t dL_doutput[OUT_LINEAR_LENGTH] = {0};
    fm_t doutput_dinput[OUT_LINEAR_LENGTH] = {0};
    fm_t dL_dw;
    fm_t grad;
    fm_t learning_rate = 0.005; 

    // Compute the derivative of the cross-entropy loss with respect to the linear layer output
    for (int i = 0; i < OUT_LINEAR_LENGTH; i++) 
    {
        dL_doutput[i] = output_feature_map[i] - target_output[i];
    }
    std::cout << std::endl << "Updated linear layer parameters" << std::endl;
    for (int j = 0; j < OUT_LINEAR_LENGTH; j++) 
    {
        for (int k = 0; k < IN_LINEAR_LENGTH; k++) 
        {
            grad = dL_doutput[j] * linear_input[k];
            linear_weights[k][j] = linear_weights[k][j] - (learning_rate * grad);

            std::cout << (float)linear_weights[k][j] << "  ";
        }
        std::cout << std::endl;
	std::cout << "----------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
    }
}
void calculateMSE(
    fm_t target_output[OUT_LINEAR_LENGTH],
    fm_t output_feature_map[OUT_LINEAR_LENGTH]
)
{   
    fm_t mse = 0;
    std::cout << "Difference between actual and predicted probabilities" << std::endl;
    for (int i = 0; i < OUT_LINEAR_LENGTH; i++)
    {
        
        std::cout << target_output[i] - output_feature_map[i] << std::endl;
        #ifdef CSIM_DEBUG
        mse += pow((target_output[i] - output_feature_map[i]), 2);
        #else
        mse += (target_output[i] - output_feature_map[i]) * (target_output[i] - output_feature_map[i]);
        #endif
    }
    mse = mse / OUT_LINEAR_LENGTH;
    std::cout << "MSE: " << mse << std::endl; 
}

