#include "const.h"

#define LARGE_NUM	(6553600.0)

void deinterleaver(vector<u32> &bit);

void deconvolution(u32 *rec_bit, vector<u32> &bit);

void init_table(int table[][BITN]);

void output_calculator(u32 state, u32 in_bit, vector<u32> &output_bit);

int hamming_distance(vector<u32> &bit1, vector<u32> &bit2);

void receiver(vector<Complex> &signal, vector<u32> &bit)
{
	OFDM_demodulator(signal, bit);
}

int MLE1(Complex &symbol)
{
	int decision = 0;
	int i = 0;
	double current = 0.0;
	double minimun = LARGE_NUM;
	/* Maximun Likelihood Estimator */
	for (i = 0; i < SYMBOL; i++)
	{
		current = distance(symbol, sym2sgnl1[i]);
		if (current <= minimun)
		{
			minimun = current;
			decision = i;
		}
	}

	return decision;
}

void deinterleaver(vector<u32> &bit)
{
	vector<u32> temp(BITN * 2);
	int count1 = 0, count2 = 0, count3 = 0;
	// read
	for (count1 = 0; count1 < 8; count1++)
	{
		for (count2 = 0; count2 < 32; count2++)
		{
			temp[count3] = bit[count2 * 8 + count1];
			count3++;
		}
	}
	// copy result to bit
	for (count1 = 0; count1 < (BITN * 2); count1++)
	{
		bit[count1] = temp[count1];
	}
}

void init_table(int table[][BITN])
{
	// 全部初始化为-1
	int i = 0, j = 0;
	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < (BITN); j++)
		{
			table[i][j] = 65536;
		}
	}
}

void output_calculator(u32 state, u32 in_bit, vector<u32> &out_bit)
{
	/*
	 * state 	-> present state of decoder
	 * in_bit 	-> 0/1
	 * out_bit 	-> output bit of decoder
	 */
	u32 pre_bit[6];
	int i = 0;
	for (i = 0; i < 6; i++)
	{
		pre_bit[i] = state % 2;
		state = state >> 1;
	}
	out_bit[0] = in_bit ^ pre_bit[1] ^ pre_bit[2] ^ pre_bit[4] ^ pre_bit[5];
	out_bit[1] = in_bit ^ pre_bit[0] ^ pre_bit[1] ^ pre_bit[2] ^ pre_bit[5];
}

int hamming_distance(vector<u32> &bit1, vector<u32> &bit2)
{
	int distance = 0;

	for (int i = 0; i < 2; i++)
	{
		if (bit1[i] != bit2[i])
			distance++;
	}

	return distance;
}

void deconvolution(vector<u32> &rec_bit, vector<u32> &bit)
{
	// define varible
	int path[64][BITN], metric[64][BITN];	// path用于记录前一个状态，metric用于记录之前的汉明距离
	init_table(path);
	init_table(metric);
	int b_pointer = 0; // bit pointer
	int s_pointer = 0;
	vector<u32> out_bit(2);
	vector<u32> temp_bit(2);
	// calculate table
	for (b_pointer = 0; b_pointer < (BITN); b_pointer++)	// bit pointer
	{
		if (b_pointer == 0)
		{	// 初始化比较特殊只有两个
			for (int z = 0; z < 2; z++)
			{
				output_calculator(0, z, out_bit);
				temp_bit[0] = rec_bit[b_pointer * 2];
				temp_bit[1] = rec_bit[b_pointer * 2 + 1];
				metric[z][b_pointer] = hamming_distance(out_bit, temp_bit);
				path[z][b_pointer] = 0;
			}
		}
		else
		{
			for (s_pointer = 0; s_pointer < 64; s_pointer++)	// 每一个状态遍历一遍 state pointer
			{
				if (metric[s_pointer][b_pointer - 1] != 65536)		// 如果之前这个状态已经有路径了
				{
					// 0
					for (int z = 0; z < 2; z++)
					{
						output_calculator(s_pointer, z, out_bit);
						temp_bit[0] = rec_bit[b_pointer * 2];
						temp_bit[1] = rec_bit[b_pointer * 2 + 1];
						if ((hamming_distance(out_bit, temp_bit) + metric[s_pointer][b_pointer - 1]) 
								< metric[((s_pointer << 1) & (0b111111)) + z][b_pointer])
						{
							metric[((s_pointer << 1) & (0b111111)) + z][b_pointer] = hamming_distance(out_bit, temp_bit)
																	+ metric[s_pointer][b_pointer - 1];
							// 记录转移过来的状态
							path[((s_pointer << 1) & (0b111111)) + z][b_pointer] = s_pointer;
						}
					}
				}
			}
		}
	}
	// 回溯
	int state = 0; // 末尾编码器状态一定是0，有清洗比特
	for (b_pointer = (BITN - 1); b_pointer >= 0; b_pointer--)
	{
		bit[b_pointer] = (state & 0b0001);
		state = path[state][b_pointer];
	}
}

int MLE2(Complex &symbol)
{
	int decision = 0;
	int i = 0;
	double current = 0.0;
	double minimun = LARGE_NUM;
	/* Maximun Likelihood Estimator */
	for (i = 0; i < SYMBOL; i++)
	{
		current = distance(symbol, sym2sgnl2[i]);
		if (current <= minimun)
		{
			minimun = current;
			decision = i;
		}
	}

	return decision;
}

void coherent_demodulator(vector<Complex> &signal, vector<u32> &bit)
{

}

void non_coherent_demodulator(vector<Complex> &signal, vector<u32> &bit)
{

}

void OFDM_demodulator(vector<Complex> &signal, vector<u32> &bit)
{
	Complex r[SYMBOLN];		/* receiverd signal */
	Complex R[SYMBOLN];		/* after DFT */
	Complex H[SYMBOLN];		/* channel state information */
	vector<u32> rec_bit(BITN * 2);
	Complex temp = {0.0, 0.0};
	int m = 0, i = 0, k = 0;
	double norm = 0.0;
	/* remove GI */
	for (i = 0; i < SYMBOLN; i++)
	{
		r[i] = signal[GI + i];
	}
	/* DFT, get Rm */
	for (m = 0; m < SYMBOLN; m++)
	{
		temp.set(0.0, 0.0);
		for (k = 0; k < SYMBOLN; k++)
		{
			temp += r[k] * Exp(-2 * PI * m * k / SYMBOLN);
		}
		R[m] = ((double)1 / sqrt(SYMBOLN)) * temp;
	}

	if (strcmp(CHANNEL, "AWGN") != 0)
	{
		for (m = 0; m < SYMBOLN; m++)
		{
			temp.set(0.0, 0.0);
			/* like DFT */
			temp += h[0];
			if (PATH_NUMBER == 2)
			{
				temp += h[1] * Exp(-2 * PI * m * DELAY / SYMBOLN);
			}
			/* give value */
			H[m] = temp;
		}
		/* channel equalizer */
		for (m = 0; m < SYMBOLN; m++)
		{
			/* initialize */
			temp.set(0.0, 0.0);
			norm = H[m].norm();
			/* equalize */
			temp = R[m] * H[m].conj();
			R[m] = temp * (1 / norm);
		}
	}
	/* symbol desision */
	for (m = 0; m < SYMBOLN; m++) 
	{
		switch (MLE1(R[m]))
		{
			case 0: rec_bit[m * 2] = 0; rec_bit[m * 2 + 1] = 0; break;
			case 1: rec_bit[m * 2] = 0; rec_bit[m * 2 + 1] = 1; break;
			case 2: rec_bit[m * 2] = 1; rec_bit[m * 2 + 1] = 1; break;
			case 3: rec_bit[m * 2] = 1; rec_bit[m * 2 + 1] = 0; break;
		}
	}
	// de-interleaver
	deinterleaver(rec_bit);
	// convolution
	deconvolution(rec_bit, bit);
}