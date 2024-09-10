// ConsoleApplication1.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
#include "stdafx.h"

using namespace std;

#include <iostream>
#include <fstream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

namespace bpt = boost::property_tree;

string culcKey(char step, string alter, string octave);
string culcNoteVal(string type);
string culcPhoVal(int Gate);
string getDyn(int dyn);
string culcTime(int Bar, int Beats, int duration, int Divisions, int BeatType);
string culcTime2(int duration, int Divisions);
void defHighKey(void);
void exchange(int* a, int* b);

#define tempo 60//StepとGateの秒数に関係する
#define arrayL 100000//配列の大きさ

int Key[arrayL], Gate[arrayL], Time[arrayL], Bar[arrayL],
Remain[arrayL], part[arrayL], tied[arrayL],//Remainは出力するかどうか（１が出力。）tieは記号がついているかどうか
Dyn[arrayL];
//arrayLは配列長さ。保存する音符の数。メモリの関係上、小さいほうが良いかも。

int main()
{
	string filename = "Chopin_-_Nocturne_Op._9_No._2";//二度うちを省略するため

	ofstream outputfile(filename + ".csv");//csv出力するファイル
	ofstream outputfile2(filename + "_NoRest.csv");//csv出力するファイル(休符無しVer)

	bpt::ptree pt;//解析のコマンドが使えるポインタ
	const int flag = bpt::xml_parser::no_comments;//コメントを解析しないようにする設定
	read_xml(filename + ".xml", pt, flag);//ここに解析するxmlファイルを代入する

	int id, BarNum;//音の番号を示す数値

	//ここで数値変数のパラメータの初期化を行う
	for (id = 0; id < arrayL; id++) {
		Key[id] = 0, Gate[id] = 0, Time[id] = 0, Bar[id] = 0,//演奏に必要なパラメータ
			part[id] = 0, Remain[id] = 1, tied[id] = 0,
			Dyn[id];//右手、最高音,タイのチェック
	}
	id = 0;

	char step;//ドレミファソラシド
	string octave;//オクターブ
	string alter;//半音上がるかどうかの数値
	string type;//音符の種類（Gateのもとになるもの）
	string strdyn;//強弱記号(読み込みの時に使用。出力は別の変数を使用)

	int duration, tmpdur;//小節内での位置(tmpdurは仮で保存しておく変数)
	int Beats, BeatType, Divisions;//Step計算用
	double dot = 1, renpuA = 1, renpuB = 1;//Gate計算用(付点,連符を考慮)
	int rest = 0;//休符判定用
	int tmpKey, tmpGate, tmpid, tmpTime = 0;//一時保存用(タイの判断用)
	int auftakt = 0;//アウフタクト(0小節)の有無を判定。1がアウフタクト有。
	int i, j;//便利に使う用


	//xmlの値取得ここから
	//1ループ1小節
	BOOST_FOREACH(bpt::ptree::value_type const& node_meas, pt.get_child("score-partwise.part"))
	{
		bpt::ptree tree_meas = node_meas.second;

		//小節番号を取得する
		if (boost::optional<std::string> str = tree_meas.get_optional<std::string>("<xmlattr>.number")) {
			if (stoi(str.get()) == 0)auftakt = 1;//アウフタクトの時、0小節目がある
			BarNum = stoi(str.get()) + auftakt;//アウフタクトの時はすべての小節番号を＋1する
		}

		if (id != 0) tmpTime = tmpTime + stoi(culcTime2(duration, Divisions));//小節最初のTimeを格納(1小節内はこれを基準にTimeを算出)
		duration = 0;//小節の最初に値をリセット(右手から左手になるときも同様)

		//1ループ1音
		BOOST_FOREACH(bpt::ptree::value_type const& node_meachi, tree_meas.get_child(""))
		{
			//拍子判定
			if (node_meachi.first == "attributes") {//attributesのラベルの中のみを見る
				bpt::ptree tree_attr = node_meachi.second;

				BOOST_FOREACH(bpt::ptree::value_type const& node_time, tree_attr.get_child(""))
				{
					bpt::ptree tree_time = node_time.second;
					//拍子を取得する
					if (boost::optional<std::string> str = tree_time.get_optional<std::string>("beats")) {
						Beats = stoi(str.get());//拍子分子
					}
					if (boost::optional<std::string> str = tree_time.get_optional<std::string>("beat-type")) {
						BeatType = stoi(str.get());//拍子分母
					}

				}
				//楽譜の最小長さの指定（4分の～となっていて、dividsionが２だったら、最小長さは８分音符の長さ）
				if (boost::optional<std::string> str = tree_attr.get_optional<std::string>("divisions")) {
					Divisions = stoi(str.get());
				}
			}

			//強弱情報
			if (node_meachi.first == "direction") {//directionのラベルの中のみを見る
				bpt::ptree tree_dir = node_meachi.second;
				//forwardの中だけを見る
				if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.pp"))Dyn[id] = 7;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.p"))Dyn[id] = 6;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.mp"))Dyn[id] = 5;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.mf"))Dyn[id] = 4;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.f"))Dyn[id] = 3;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.ff"))Dyn[id] = 2;
				else if (boost::optional<string> str = tree_dir.get_optional<string>("direction-type.dynamics.sf"))Dyn[id] = 9;
			}

			//音符情報
			if (node_meachi.first == "note") {//noteのラベルの中のみを見る

				dot = 1;//付点の初期化
				renpuA = 1, renpuB = 1;//連符の初期化
				rest = 0;//休符判定初期化
				alter = "0";//半音判定初期化

				bpt::ptree tree_note = node_meachi.second;

				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("chord"))//同時打鍵
					duration -= tmpdur;

				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("rest")) {//休符
					Key[id] = 0;
				}
				else if (boost::optional<std::string> str = tree_note.get_optional<std::string>("pitch")) {
					BOOST_FOREACH(bpt::ptree::value_type const& node_pitch, tree_note.get_child(""))
					{
						bpt::ptree tree_pitch = node_pitch.second;
						//音の高さを取得する
						if (boost::optional<std::string> str = tree_pitch.get_optional<std::string>("step")) {
							step = *str.get().c_str();
						}
						if (boost::optional<std::string> str = tree_pitch.get_optional<std::string>("octave")) {
							octave = str.get();
						}
						if (boost::optional<std::string> str = tree_pitch.get_optional<std::string>("alter")) {
							alter = str.get();
						}
					}
					Key[id] = stoi(culcKey(step, alter, octave));
				}
				//Time[id] = stoi(culcTime(BarNum, Beats, duration, Divisions, BeatType));
				if (id != 0)Time[id] = tmpTime + stoi(culcTime2(duration, Divisions));

				//音符の位置と種類を取得する
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("duration")) {
					tmpdur = stoi(str.get());
					duration += tmpdur;
				}

				//付点の有無を判定
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("dot"))
					dot = 1.5;//今のところ付点は一つのみを想定

				//音符種類の変換
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("type")) {
					type = str.get();
				}

				//右手(1)か左手(2)の決定
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("staff")) {
					if (str.get() == "1")part[id] = 2;//右手
					if (str.get() == "2")part[id] = 5;//左手
				}

				//連符の判定
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("time-modification")) {
					BOOST_FOREACH(bpt::ptree::value_type const& node_timemod, tree_note.get_child(""))
					{
						bpt::ptree tree_timemod = node_timemod.second;
						//連符の、1音の数と2拍数を得る
						if (boost::optional<std::string> str = tree_timemod.get_optional<std::string>("actual-notes")) {
							renpuA = stoi(str.get());
						}
						if (boost::optional<std::string> str = tree_timemod.get_optional<std::string>("normal-notes")) {
							renpuB = stoi(str.get());
						}
					}
					renpuA = renpuB / renpuA;
				}

				//Gateの計算(音符種類等を考慮して、テンポで変換している)
				Gate[id] = int(stod(culcNoteVal(type))*dot*renpuA);

				Bar[id] = BarNum;


				//タイ、スラーの発見
				if (boost::optional<std::string> str = tree_note.get_optional<std::string>("notations")) {
					//タイの時
					if (boost::optional<std::string> str = tree_note.get_optional<std::string>("notations.tied")) {

						boost::optional<std::string> str2 = tree_note.get_optional<std::string>("notations.tied.<xmlattr>.type");
						if (str2.get() == "start")tied[id] = 1;//１をstartとする
						if (str2.get() == "stop" && id != 0) {
							tied[id] = 2;//２をstopとする
							Remain[id] = 0;

							//stopのidのGateとKeyを一時保存（startの音符に用いるため）
							tmpKey = Key[id];
							tmpGate = Gate[id];
							tmpid = id - 1;

							//startの位置を発見（while文で）
							while (tmpKey != Key[tmpid]) {
								tmpid--;
							}

							//発見した音符を修正する
							Gate[tmpid] += tmpGate;
						}
					}
				}

				id++;//処理終了後にidを更新
			}

			if (node_meachi.first == "forward") {//noteのラベルの中のみを見る
				bpt::ptree tree_forward = node_meachi.second;
				//forwardの中だけを見る
				if (boost::optional<std::string> str = tree_forward.get_optional<std::string>("duration")) {
					duration += stoi(str.get());
				}
			}
			//backupの中だけを見る
			if (node_meachi.first == "backup") {//noteのラベルの中のみを見る
				bpt::ptree tree_backup = node_meachi.second;
				//forwardの中だけを見る
				if (boost::optional<std::string> str = tree_backup.get_optional<std::string>("duration")) {
					duration -= stoi(str.get());
				}
			}

		}
	}
	//xmlの値取得ここまで

	defHighKey();//最高音判定関数

	//ファイル保存
	outputfile << "Key,Gate,Velo,Time,Bar,part,PhoVal,id,Remain,Dynamic" << std::endl;//ラベル表示(ascでは不要)
	id = 0;
	int dynflag = 0;
	//int id2 = 1, leftH = 1, rightH = 1;//表示のみに使用
	int id3 = 0;//表示に用いるID

	while (Bar[id] != 0) {

		//強弱記号決定処理
		if (Dyn[id] == 0)Dyn[id] = dynflag;//強弱記号はsf以外継続とみなす
		else if (Dyn[id] != 9) dynflag = Dyn[id];

		////右手左手それぞれのメロディによるID判定処理
		//if (Remain[id] == 2 && part[id] == 2)id2 = rightH, rightH++;
		//else if (Remain[id] == 2 && part[id] == 5)id2 = leftH, leftH++;
		
		if (Remain[id] != 0)id3++;//表示するもののみIDを更新

		if (Remain[id] != 0) {
			outputfile << Key[id]
				<< "," << Gate[id]
				<< ",60,"
				<< Time[id]
				<< "," << Bar[id]
				<< "," << part[id]
				<< "," << culcPhoVal(Gate[id])
				<< "," << to_string(id3)
				<< "," << Remain[id]
				<< "," << getDyn(Dyn[id])
				<< std::endl;
		}
		id++;
	}
	outputfile.close();//ファイル1書き込み終了

	//ファイル保存その2
	outputfile2 << "Key,Gate,Velo,Time,Bar,part,PhoVal,id,Remain,Dynamic" << std::endl;//ラベル表示(ascでは不要)
	id = 0;
	dynflag = 0;
	//id2 = 1, leftH = 1, rightH = 1;//表示のみに使用
	id3 = 0;

	while (Bar[id] != 0) {

		//強弱記号決定処理
		if (Dyn[id] == 0)Dyn[id] = dynflag;//強弱記号はsf以外継続とみなす
		else if (Dyn[id] != 9) dynflag = Dyn[id];

		////右手左手それぞれのメロディによるID判定処理
		//if (Remain[id] == 2 && part[id] == 2)id2 = rightH, rightH++;
		//else if (Remain[id] == 2 && part[id] == 5)id2 = leftH, leftH++;

		if (Remain[id] != 0)id3++;//表示するもののみIDを更新(key=0も含める)

		if (Remain[id] != 0 && Key[id] != 0) {
			outputfile2 << Key[id]
				<< "," << Gate[id]
				<< ",60,"
				<< Time[id]
				<< "," << Bar[id]
				<< "," << part[id]
				<< "," << culcPhoVal(Gate[id])
				<< "," << to_string(id3)
				<< "," << Remain[id]
				<< "," << getDyn(Dyn[id])
				<< std::endl;
		}
		id++;
	}
	outputfile2.close();//書き込み終了


}

//keyをMIDIの表記に変	換する関数
string culcKey(char step, string alter, string octave) {
	int key;
	int st = 0, al = 0, oc;
	if (octave != "")oc = stoi(octave);
	if (alter != "")al = stoi(alter);
	switch (step)
	{
	case 'C':st = 0; break;//ド
	case 'D':st = 2; break;//レ
	case 'E':st = 4; break;//ミ
	case 'F':st = 5; break;//ファ
	case 'G':st = 7; break;//ソ
	case 'A':st = 9; break;//ラ
	case 'B':st = 11; break;//シ
	default:
		break;
	}
	key = (oc + 1) * 12 + st + al;
	return to_string(key);
}

//音符の種類からMIDIの数値に変換する関数
string culcNoteVal(string type) {
	double NoteVal = -1;//何もなかった時、エラーとして返す

	if (type.compare("256th") == 0)NoteVal = 7.5;//256分音符
	if (type.compare("128th") == 0)NoteVal = 15;//128分音符
	if (type.compare("64th") == 0)NoteVal = 30;//64分音符
	if (type.compare("32nd") == 0)NoteVal = 60;//32分音符
	if (type.compare("16th") == 0)NoteVal = 120;//16分音符
	if (type.compare("eighth") == 0)NoteVal = 240;//8分音符
	if (type.compare("quarter") == 0)NoteVal = 480;//4分音符
	if (type.compare("half") == 0)NoteVal = 960;//2分音符
	if (type.compare("whole") == 0)NoteVal = 1920;//全音符
	if (type.compare("breve") == 0)NoteVal = 3840;//2倍全音符
	if (type.compare("long") == 0)NoteVal = 7680;//4倍全音符
	if (type.compare("maxima") == 0)NoteVal = 15360;//8倍全音符

	return to_string(NoteVal);
}

//音符の種類からMIDIの数値に変換する関数
string getDyn(int dyn) {
	string dynval;

	if (dyn == 0)dynval = "non";
	if (dyn == 1)dynval = "fff";
	if (dyn == 2)dynval = "ff";
	if (dyn == 3)dynval = "f";
	if (dyn == 4)dynval = "mf";
	if (dyn == 5)dynval = "mp";
	if (dyn == 6)dynval = "p";
	if (dyn == 7)dynval = "pp";
	if (dyn == 8)dynval = "ppp";
	if (dyn == 9)dynval = "sf";

	return dynval;
}

//Gateを記号(1文字)に変換する関数
string culcPhoVal(int Gate) {
	string PhoVal = "0";

	if (Gate == 1920)PhoVal = "w";//whole(全音符)
	if (Gate == 960)PhoVal = "h";//half(2分音符)
	if (Gate == 480)PhoVal = "q";//quarter(4分音符)
	if (Gate == 240)PhoVal = "e";//eighth(8分音符)
	if (Gate == 120)PhoVal = "s";//16th(16分音符)
	if (Gate == 60)PhoVal = "t";//32nd(32分音符)

	if (Gate == 2880)PhoVal = "a";//付点全音符
	if (Gate == 1440)PhoVal = "b";//付点2分音符
	if (Gate == 720)PhoVal = "c";//付点4分音符
	if (Gate == 360)PhoVal = "d";//付点8分音符
	if (Gate == 180)PhoVal = "f";//付点16分音符

	if (Gate == 640)PhoVal = "u";//全音符の3連符
	if (Gate == 320)PhoVal = "v";//2分音符の3連符
	if (Gate == 160)PhoVal = "x";//4分音符の3連符
	if (Gate == 80)PhoVal = "y";//8分音符の3連符
	if (Gate == 40)PhoVal = "z";//16分音符の3連符


	return PhoVal;
}

//Time算出関数(位置で算出)
string culcTime(int Bar, int Beats, int duration, int Divisions, int BeatType) {
	return to_string(int(((Bar - 1)*Beats + double(duration) / Divisions * BeatType / 4) * (1920 / BeatType) * 60 / tempo));
}

//Time算出関数(変化分のみを算出)
string culcTime2(int duration, int Divisions) {
	return to_string((double(duration) / Divisions) * 480 * (60 / tempo));
}

//最高音判定関数
void defHighKey(void) {
	int i = 0, j = 1, k;

	//Time順に並べ替え
	while (Bar[i] != 0)
	{
		j = i + 1;
		while (Bar[j] != 0)
		{
			if (Time[i] > Time[j]) {
				exchange(&Key[i], &Key[j]);
				exchange(&Gate[i], &Gate[j]);
				exchange(&Time[i], &Time[j]);
				exchange(&Bar[i], &Bar[j]);
				exchange(&part[i], &part[j]);
				exchange(&Dyn[i], &Dyn[j]);
				exchange(&Remain[i], &Remain[j]);
			}
			j++;
		}
		i++;
	}
	i = 0;//iの初期化

	int tmpTime = 0;
	int tmpID = 0;

	//同Timeの中でKeyの高い順に並べ替え
	while (Bar[i] != 0)
	{
		tmpTime = Time[i];
		tmpID = i;

		while (Time[i] == tmpTime) {
			i++;
		}
		for (j = tmpID; j < i; j++) {
			for (k = j + 1; k < i; k++) {
				if (Key[j] < Key[k]) {
					exchange(&Key[j], &Key[k]);
					exchange(&Gate[j], &Gate[k]);
					exchange(&Time[j], &Time[k]);
					exchange(&Bar[j], &Bar[k]);
					exchange(&part[j], &part[k]);
					exchange(&Dyn[j], &Dyn[k]);
					exchange(&Remain[j], &Remain[k]);
				}
				else if (part[j] == part[k] && (Key[j] == Key[k])) {//同時に同じ音を複数演奏する場合（楽譜上正しくても、演奏は不可）
					if (Gate[j] < Gate[k])Remain[j] = 0;
					else Remain[k] = 0;
				}
			}
		}
	}

	////最高音だけを残す////
	i = 1;
	int right = 0, left = 0;

	//最初の音の処理
	Remain[0] = 2;//最初は必ず最高音(ソートしている為)
	if (part[0] == 2)right = 1;
	else if (part[0] == 5)left = 1;

	//二音目以降の処理
	while (Bar[i] != 0)
	{
		if (Remain[i] != 0) {//Remain=0のものは無視
			j = i - 1;
			while (Remain[j] == 0)j--;//Remain=0のものを除いた一つ前の音まで戻る(基本は一つ前のID)
			if (Time[i] != Time[j])right = 0, left = 0;//同時打鍵が更新されたかどうかの判断
			if (part[i] == 2 && right == 0) {
				Remain[i] = 2, right = 1;//Remain=2は最高音。rightに1を代入することで最高音の重複を防ぐ
			}
			else if (part[i] == 5 && left == 0) {
				Remain[i] = 2, left = 1;//Remain=2は最高音。leftに1を代入することで最高音の重複を防ぐ
			}
		}
		i++;
	}
}

//交換関数
void exchange(int *a, int *b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}
