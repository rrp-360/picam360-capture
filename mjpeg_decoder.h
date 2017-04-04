#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void init_mjpeg_decoder(int cam_num, void *user_data);
void deinit_mjpeg_decoder(int cam_num);
float mjpeg_decoder_get_fps(int cam_num);
int mjpeg_decoder_get_frameskip(int cam_num);
void mjpeg_decode(int cam_num, unsigned char *data, int data_len);

#ifdef __cplusplus
}
#endif