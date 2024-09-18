from music21 import converter, note, chord, stream

import csv

# 四分音符の長さを1拍=500msと仮定（テンポ120）
quarter_note_length_ms = 500

# MusicXMLファイルの読み込み
score = converter.parse('Chopin_-_Nocturne_Op._9_No._2.xml')

# CSVファイルの作成
with open('output2.csv', 'w', newline='') as csvfile:
    csvwriter = csv.writer(csvfile)
    csvwriter.writerow(['Key', 'Gate', 'Velo', 'Time', 'Bar', 'Part', 'Pitch', 'Octave', 'Duration', 'Dynamic'])

    # パートごとの処理
    for part_index, part in enumerate(score.parts):
        current_time_ms = 0  # 演奏開始からの時間 (ms)
        
        for element in part.flatten().notes:
            if isinstance(element, note.Note):
                # Key (音の高さ) - MIDIピッチ番号
                key = element.pitch.midi
                
                # Gate (音の長さ) - msに変換
                gate = element.quarterLength * quarter_note_length_ms
                
                # Velo (音の強さ) - MIDIのベロシティ
                velocity = element.volume.velocity if element.volume.velocity is not None else 64  # デフォルト値64
                
                # Time (開始時間) - 演奏開始からのms
                time = current_time_ms
                
                # Bar (小節の位置)
                bar = element.measureNumber
                
                # Dynamics - 強弱記号
                dynamics = 'mf'  # デフォルトの強弱記号
                for exp in element.expressions:
                    if hasattr(exp, 'content'):
                        dynamics = exp.content
                        break

                # Pitch, Octave, Duration
                pitch = element.name
                octave = element.octave
                duration = element.quarterLength

                # 書き込み
                csvwriter.writerow([key, gate, velocity, time, bar, part_index + 1, pitch, octave, duration, dynamics])

            elif isinstance(element, chord.Chord):
                # Key (音の高さ) - 和音の各ピッチのMIDI番号を取得
                key = '.'.join(str(n.midi) for n in element.pitches)
                
                # Gate (音の長さ) - msに変換
                gate = element.quarterLength * quarter_note_length_ms
                
                # Velo (音の強さ) - 和音の場合は最初の音符のベロシティを使用
                velocity = element.volume.velocity if element.volume.velocity is not None else 64  # デフォルト値64
                
                # Time (開始時間) - 演奏開始からのms
                time = current_time_ms
                
                # Bar (小節の位置)
                bar = element.measureNumber
                
                # Dynamics - 強弱記号
                dynamics = 'mf'
                for exp in element.expressions:
                    if hasattr(exp, 'content'):
                        dynamics = exp.content
                        break

                # Pitch, Duration (和音にはオクターブはなし)
                pitch = '.'.join(n.nameWithOctave for n in element.pitches)
                duration = element.quarterLength

                # 書き込み
                csvwriter.writerow([key, gate, velocity, time, bar, part_index + 1, pitch, '', duration, dynamics])
            
            # Timeを更新
            current_time_ms += gate
