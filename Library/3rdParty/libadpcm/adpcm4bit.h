#ifndef _ADPCM_4BIT_H
#define _ADPCM_4BIT_H
//-----------------------------------------------------------------------------

/*
    Function: 
        AdpcmEnc4
    
    Description:
        This function is used to encode input 16-bit mono PCM data to 4-bit ADPCM bit-stream.
        The output size of bit-stream is dependent on the number of input samples. The formula 
        is: bit-stream size = number of input samples/2 + 4.
        
        ex: if input sample number is 120, then the output bit-stream size is (120/2 + 4) = 64 bytes.
        
    Parameters:
        ibuff   [in]    The input 16-bit mono PCM data.
        n       [in]    The number of samples in ibuff, it must be a multiplayer of 4.
        st      [in/out]The state of encoder. This variable is used to keep encoder state whenever calling AdpcmEnc4
        obuff   [out]   The buffer to store the 4-bit ADPCM bit-stream. The size of obuff is dependent on number of input samples
    
    return:
       None

*/
void AdpcmEnc4(short *ibuff, int n, int *st, unsigned char *obuff);


/*
    Function: 
        AdpcmDec4
    
    Description:
        This function is used to decode input 4-bit ADPCM bit-stream which created by AdpcmEnc4.
        It is necessary to know the relative parameters of encoder when calling it to decode the bit-stream.
        For example, if the bit-stream is encoded with 120 sample number, then the bit-stream size should be 
        120/2+4 = 64 bytes, and the number of decoded out samples are also 120. 
        
    Parameters:
        ibuff   [in]    The input bit-stream buffer size. It must match the setting of encoder.
        obuff   [out]   The buffer to store the output PCM data. The size of obuff is dependent on encoder.
        n       [in]    The number of samples in obuff
    
    return:
        None
*/
void AdpcmDec4(unsigned char *ibuff, short *obuff, int n);

//-----------------------------------------------------------------------------
#endif
