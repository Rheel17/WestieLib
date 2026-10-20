#!/bin/bash

DIR="/media/wcs"
DL_DIR="$DIR/dl"
MUSIC_DIR="$DIR/music"
OVERWRITE_DIR="$DIR/overwrite"
MANIFEST_FILE="$DIR/manifest.txt"
META_FILE="$DIR/meta.txt"
PLAYLIST_FILE="$DIR/playlist.txt"
REPLACE_FILE="$DIR/replace.json"
YTDLP="/opt/YoutubeDownloader/yt-dlp_linux"

PLAYLIST="https://www.youtube.com/playlist?list=PLGXm7qiSwxfBEFj0x0Kv4n2m959q-uCoM"

mkdir -p "$DL_DIR"
mkdir -p "$MUSIC_DIR"

if [ ! -f "$META_FILE" ]; then
    touch "$META_FILE"
fi

echo "GET PLAYLIST TRACKS"
"$YTDLP" -j --flat-playlist "$PLAYLIST" | jq -r '.id' | sort > "$PLAYLIST_FILE"
echo "$(wc -l < "$PLAYLIST_FILE") TRACKS IN PLAYLIST"
echo

echo "FIND UNDOWNLOADED TRACKS"
comm -3 "$PLAYLIST_FILE" "$META_FILE" | grep -v '^\s' > "$MANIFEST_FILE"
echo "FOUND $(wc -l < "$MANIFEST_FILE") TRACKS"
echo

download_track () {
    local id
    local success
    id="$1"

    echo "DOWNLOADING $id"

    dlid=$(jq -r --arg id "$id" '.[$id] // $id' "$REPLACE_FILE")

    if [ "$id" != "$dlid" ]; then
        echo "DOWNLOAD ID: $dlid"
    fi

    # Download
    "$YTDLP" \
       --quiet \
       --format bestaudio \
       --extract-audio \
       --audio-format flac \
       --write-thumbnail \
       --convert-thumbnail jpg \
       --write-info-json \
       --output "$DL_DIR/%(id)s.%(ext)s" \
       --windows-filenames \
       --extractor-args "youtube:player_client=default" \
       "https://www.youtube.com/watch?v=$dlid"
    success=$?

    if [ "$success" -ne 0 ]; then
        echo "Downloading $id failed!"
        return
    fi

    local img
    local info
    local flac

    local img="$DL_DIR/$dlid.jpg"
    local info="$DL_DIR/$dlid.info.json"
    local flac="$DL_DIR/$dlid.flac"

    if [ -f "$OVERWRITE_DIR/$dlid.jpg" ]; then
        img="$OVERWRITE_DIR/$dlid.jpg"
    fi

    if [ -f "$OVERWRITE_DIR/$dlid.info.json" ]; then
        info="$OVERWRITE_DIR/$dlid.info.json"
    fi

    if [ -f "$img" ]; then
        # Crop cover to square
        ffmpeg \
            -i "$img" \
            -vf "crop=min(iw\,ih):min(iw\,ih)" \
            -f image2 \
            -y "${img%.jpg}_cropped.jpg" \
            -loglevel quiet

        mv "${img%.jpg}_cropped.jpg" "$img"

        # Embed cover
        metaflac --import-picture-from="$img" "$flac"
    fi

    # Add metadata
    local artist
    local album
    local title

    artist=$(jq -r '.artist' "$info") 
    album=$(jq -r '.album' "$info")
    title=$(jq -r '.fulltitle' "$info")


    echo "New track: $artist [$album]: $title"

    metaflac --set-tag=artist="$artist" "$flac"
    metaflac --set-tag=album="$album" "$flac"
    metaflac --set-tag=title="$title" "$flac"

    # Create the link in the music dir
    ln -s "$flac" "$MUSIC_DIR/$artist - $title.flac"

    # Output track to the meta
    sort -m "$META_FILE" <(echo "$id") -o "$META_FILE"
}

mapfile -t tracks < "$MANIFEST_FILE"

for track in "${tracks[@]}"; do
    download_track "$track"
done

