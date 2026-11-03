#!/usr/bin/env python3
import os
import subprocess

BUILD_DIR = "/Users/ishara/Other Projects/Esp32/promo_video_build"
SEGS_DIR = f"{BUILD_DIR}/segments"
REC_DIR = "/Users/ishara/Other Projects/Esp32/promo_video_recordings"
CARDS_DIR = f"{BUILD_DIR}/cards"
FINAL_OUTPUT = "/Users/ishara/Other Projects/Esp32/auraflow_promo_30s.mp4"

os.makedirs(SEGS_DIR, exist_ok=True)

def run_cmd(cmd):
    print("Executing:", " ".join(cmd[:6]), "...")
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if res.returncode != 0:
        print("ERROR:", res.stderr.decode()[-600:])
        raise RuntimeError("FFmpeg command failed")

# 1. Segment 1: Opening Hook Card (2.5s)
seg1 = f"{SEGS_DIR}/seg01.mp4"
run_cmd([
    "ffmpeg", "-y", "-loop", "1", "-i", f"{CARDS_DIR}/shot1.png",
    "-t", "2.5",
    "-vf", "scale=1920:1080,format=yuv420p",
    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", seg1
])

# 2. Segment 2: Giant Clock Hero Amber (2.5s)
seg2 = f"{SEGS_DIR}/seg02.mp4"
run_cmd([
    "ffmpeg", "-y", "-ss", "00:00:01", "-i", f"{REC_DIR}/shot2_clock_amber.mp4",
    "-i", f"{CARDS_DIR}/title_shot2.png",
    "-t", "2.5",
    "-filter_complex",
    "[0:v]crop=1440:665:0:1227,scale=1920:886,pad=1920:1080:(ow-iw)/2:(oh-ih)/2:black[vid];"
    "[vid][1:v]overlay=0:0[final]",
    "-map", "[final]", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", seg2
])

# 3. Segment 2 Montage: 4 Theme Flashes (0.5s each = 2.0s)
seg2_montage = f"{SEGS_DIR}/seg03.mp4"
theme_clips = [
    (f"{REC_DIR}/shot2b_clock_cyan.mp4", "00:00:01", "0.5"),
    (f"{REC_DIR}/shot2c_clock_purple.mp4", "00:00:01", "0.5"),
    (f"{REC_DIR}/shot2d_clock_white.mp4", "00:00:01", "0.5"),
    (f"{REC_DIR}/shot2_clock_amber.mp4", "00:00:02.5", "0.5"),
]
montage_parts = []
for idx, (path, ss, dur) in enumerate(theme_clips):
    m_part = f"{SEGS_DIR}/theme_{idx}.mp4"
    run_cmd([
        "ffmpeg", "-y", "-ss", ss, "-i", path,
        "-t", dur,
        "-vf", "crop=1440:665:0:1227,scale=1920:886,pad=1920:1080:(ow-iw)/2:(oh-ih)/2:black,format=yuv420p",
        "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", m_part
    ])
    montage_parts.append(m_part)

# Concat 4 theme flashes
concat_list_theme = f"{SEGS_DIR}/themes.txt"
with open(concat_list_theme, "w") as f:
    for p in montage_parts:
        f.write(f"file '{p}'\n")

run_cmd([
    "ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", concat_list_theme,
    "-c", "copy", seg2_montage
])

# 4. Segment 3: Font cycle & scale (3.0s)
seg3 = f"{SEGS_DIR}/seg04.mp4"
run_cmd([
    "ffmpeg", "-y", "-ss", "00:00:01.5", "-i", f"{REC_DIR}/shot3_font_zoom.mp4",
    "-i", f"{CARDS_DIR}/title_shot3.png",
    "-t", "3.0",
    "-filter_complex",
    "[0:v]crop=1440:665:0:1227,scale=1920:886,pad=1920:1080:(ow-iw)/2:(oh-ih)/2:black[vid];"
    "[vid][1:v]overlay=0:0[final]",
    "-map", "[final]", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", seg3
])

# Helper for portrait layout clips (phone mockup centered on blurred bg + title overlay)
def process_portrait(input_mp4, title_png, ss, dur, output_mp4):
    run_cmd([
        "ffmpeg", "-y", "-ss", ss, "-i", input_mp4,
        "-i", title_png,
        "-t", dur,
        "-filter_complex",
        "[0:v]scale=1920:1080,boxblur=25:5,eq=brightness=-0.22:contrast=0.82[bg];"
        "[0:v]scale=-1:960[phone];"
        "[bg][phone]overlay=(1920-w)/2:60[comp];"
        "[comp][1:v]overlay=0:0[final]",
        "-map", "[final]", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", output_mp4
    ])

# 5. Segment 4: Stopwatch (3.0s)
seg4 = f"{SEGS_DIR}/seg05.mp4"
process_portrait(f"{REC_DIR}/shot4_stopwatch.mp4", f"{CARDS_DIR}/title_shot4.png", "00:00:01", "3.0", seg4)

# 6. Segment 6: Pomodoro Dial (3.5s)
seg6 = f"{SEGS_DIR}/seg06.mp4"
process_portrait(f"{REC_DIR}/shot6_pomodoro_dial.mp4", f"{CARDS_DIR}/title_shot6.png", "00:00:01", "3.5", seg6)

# 7. Segment 7: RGB Remote (3.5s)
seg7 = f"{SEGS_DIR}/seg07.mp4"
process_portrait(f"{REC_DIR}/shot7_rgb_remote.mp4", f"{CARDS_DIR}/title_shot7.png", "00:00:01", "3.5", seg7)

# 8. Segment 8: GitHub Code Scroll (3.5s)
seg8 = f"{SEGS_DIR}/seg08.mp4"
process_portrait(f"{REC_DIR}/shot8_github_code_retake.mp4", f"{CARDS_DIR}/title_shot8.png", "00:00:01", "3.5", seg8)

# 9. Segment 9: Outro Card (4.5s)
seg9 = f"{SEGS_DIR}/seg09.mp4"
run_cmd([
    "ffmpeg", "-y", "-loop", "1", "-i", f"{CARDS_DIR}/shot9.png",
    "-t", "4.5",
    "-vf", "scale=1920:1080,format=yuv420p",
    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", "30", seg9
])

# Concatenate all 9 segments
all_segments = [seg1, seg2, seg2_montage, seg3, seg4, seg6, seg7, seg8, seg9]
concat_file = f"{SEGS_DIR}/all_segs.txt"
with open(concat_file, "w") as f:
    for seg in all_segments:
        f.write(f"file '{seg}'\n")

video_raw = f"{SEGS_DIR}/video_concat_raw.mp4"
run_cmd([
    "ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", concat_file,
    "-c", "copy", video_raw
])

# Mux with electronic synth soundtrack
soundtrack_wav = f"{BUILD_DIR}/synth_soundtrack.wav"
run_cmd([
    "ffmpeg", "-y", "-i", video_raw, "-i", soundtrack_wav,
    "-c:v", "copy", "-c:a", "aac", "-b:a", "192k",
    "-shortest",
    FINAL_OUTPUT
])

print("PROMO VIDEO COMPLETED SUCCESSFULLY:", FINAL_OUTPUT)
