#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

void play_video_file(const char *path, int skip_enabled);
void play_video_file_delay(const char *path, int skip_enabled, float delay_seconds);
void video_player_init();
void video_player_quit();

#ifdef __cplusplus
}
#endif

#endif // VIDEO_PLAYER_H
