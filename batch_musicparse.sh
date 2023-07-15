#!/bin/bash

low='abcdefghijklmnopqrstuvwxyz'
cap='ABCDEFGHIJKLMNOPQRSTUVWXYZ'

for i in BetterChorales/$1/Chorale*.xml; do
  SUFFIX=$(echo $i | tail -c 7)
  FILE_END=$(printf "%s" "$SUFFIX" | sed "s/[0-9]//g")
  END_LETTERS=${FILE_END%.*}
  NUM=$(printf "%03X\n" "0x$1")

  TONALITY="maj"
  KEY="a_flat_${TONALITY}"

  case $END_LETTERS in
  [$low]*)
    TONALITY="min"
    ;;
  [$cap]*)
    TONALITY="maj"
    ;;
  esac

  END_LETTERS=$(echo "$END_LETTERS" | tr '[:upper:]' '[:lower:]')

  case $END_LETTERS in
  "af")
    KEY="a_flat_${TONALITY}"
    ;;
  "a")
    KEY="a_${TONALITY}"
    ;;
  "as")
    KEY="a_sharp_${TONALITY}"
    ;;
  "bf")
    KEY="b_flat_${TONALITY}"
    ;;
  "b")
    KEY="b_${TONALITY}"
    ;;
  "cf")
    KEY="c_flat_${TONALITY}"
    ;;
  "c")
    KEY="c_${TONALITY}"
    ;;
  "cs")
    KEY="c_sharp_${TONALITY}"
    ;;
  "df")
    KEY="d_flat_${TONALITY}"
    ;;
  "d")
    KEY="d_${TONALITY}"
    ;;
  "ef")
    KEY="e_flat_${TONALITY}"
    ;;
  "e")
    KEY="e_${TONALITY}"
    ;;
  "f")
    KEY="f_${TONALITY}"
    ;;
  "fs")
    KEY="f_sharp_${TONALITY}"
    ;;
  "gf")
    KEY="g_flat_${TONALITY}"
    ;;
  "g")
    KEY="g_${TONALITY}"
    ;;
  "gs")
    KEY="g_sharp_${TONALITY}"
    ;;
  *)
    continue
    ;;
  esac

  WRITTEN_KEY="C"
  case $END_LETTERS in
  "af")
    WRITTEN_KEY="A♭"
    ;;
  "a")
    WRITTEN_KEY="A"
    ;;
  "as")
    WRITTEN_KEY="A#"
    ;;
  "bf")
    WRITTEN_KEY="B♭"
    ;;
  "b")
    WRITTEN_KEY="B"
    ;;
  "cf")
    WRITTEN_KEY="C♭"
    ;;
  "c")
    WRITTEN_KEY="C"
    ;;
  "cs")
    WRITTEN_KEY="C#"
    ;;
  "df")
    WRITTEN_KEY="D♭"
    ;;
  "d")
    WRITTEN_KEY="D"
    ;;
  "ef")
    WRITTEN_KEY="E♭"
    ;;
  "e")
    WRITTEN_KEY="E"
    ;;
  "f")
    WRITTEN_KEY="F"
    ;;
  "fs")
    WRITTEN_KEY="F#"
    ;;
  "gf")
    WRITTEN_KEY="G♭"
    ;;
  "g")
    WRITTEN_KEY="G"
    ;;
  "gs")
    WRITTEN_KEY="G#"
    ;;
  *)
    continue
    ;;
  esac

  if [ "$TONALITY" = "min" ]; then
    WRITTEN_KEY=$(echo "$WRITTEN_KEY" | tr '[:upper:]' '[:lower:]')
  fi

  FILE_OUTPUT="outputs/${KEY}/${NUM}.txt"

  if [ ! -f "$FILE_OUTPUT" ]; then
    ./musicparse $i 2 >$FILE_OUTPUT
    echo "---" >>$FILE_OUTPUT

    # Replace "K: X" with "X: KEY"
    awk -v key="$WRITTEN_KEY" '/^K: / { $0 = "K: " key } 1' "$FILE_OUTPUT" >tmp_file
    mv tmp_file "$FILE_OUTPUT"
  fi

done
