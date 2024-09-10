from music21 import converter, note, chord, stream

import csv

# MusicXMLファイルの読み込み
score = converter.parse('Chopin_-_Nocturne_Op._9_No._2.xml')

# CSVファイルの作成
with open('output.csv', 'w', newline='') as csvfile:
    csvwriter = csv.writer(csvfile)
    csvwriter.writerow(['Pitch', 'Octave', 'Duration', 'Dynamic'])

    # パートごとの処理
    for part in score.parts:
        for element in part.flatten().notes:
            if isinstance(element, note.Note):
                pitch = element.name
                octave = element.octave
                duration = element.quarterLength
                
                # dynamicsを取得する部分の修正
                dynamics = 'mf'  # デフォルトの強弱記号
                for exp in element.expressions:
                    if hasattr(exp, 'content'):
                        dynamics = exp.content
                        break

                csvwriter.writerow([pitch, octave, duration, dynamics])

            elif isinstance(element, chord.Chord):
                pitch = '.'.join(n.nameWithOctave for n in element.pitches)
                duration = element.quarterLength
                
                dynamics = 'mf'  # デフォルトの強弱記号
                for exp in element.expressions:
                    if hasattr(exp, 'content'):
                        dynamics = exp.content
                        break

                csvwriter.writerow([pitch, '', duration, dynamics])
            elif isinstance(element, note.Rest):
                csvwriter.writerow(['Rest', '', element.quarterLength, ''])
