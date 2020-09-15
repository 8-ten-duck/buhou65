#undef _DEBUG
#undef JINX_ALLOC_REBIND_NOT_USED

#include "Jinx.hpp"

#include<iostream>
#include<fstream>
#include<string>
#include<regex>
#include<cassert>

/**
 * ファイル名整形関数
 * @param [in] path ファイルへのパス
 * @param [in] suffix ファイルの拡張子
 * @return 生成したファイルへのパス
 */
const std::string asm_file_name(const std::string& path, const std::string& suffix){
    //検索結果の一覧
    std::smatch matches;
    //ファイルパスを表す正規表現
    const std::regex reg_path("(.+)(\\.)([a-zA-Z]+)$");

    //ファイルパスの要素を取得する
    const bool result = std::regex_search(path, matches, reg_path);
    assert(result);

    //サフィックスを確かめる
    if(matches.size() == 3 && matches.str(matches.size()) == suffix){
        return "";
    }

    //ファイル名を返す
    return matches.str(1) + matches.str(2) + suffix;
}

/**
 * Jinx初期化関数
 */
void init_jinx(){
    //初期化パラメータ
    Jinx::GlobalParams globalParams;

    //初期化パラメータを設定する
    globalParams.enableLogging = true;
    globalParams.logBytecode = true;
    globalParams.logSymbols = true;
    globalParams.enableDebugInfo = true;
    globalParams.logFn = [](Jinx::LogLevel level, const char * msg) { printf(msg); };
    globalParams.allocBlockSize = 1024 * 16;
    globalParams.allocFn = [](size_t size) { return malloc(size); };
    globalParams.reallocFn = [](void * p, size_t size) { return realloc(p, size); };
    globalParams.freeFn = [](void * p) { free(p); };

    //Jinxを初期化する
    Jinx::Initialize(globalParams);
}

/**
 * Jinxを終了させる
 */
void exit_jinx(){
    Jinx::ShutDown();
}

int main(int argc, char* argv[]){
    //Jinxを初期化する
    init_jinx();

    // Create the Jinx runtime object
    auto runtime = Jinx::CreateRuntime();

    if(argc < 2){
        std::cerr << "argc < 2." << std::endl;
        return 1;
    }

    //ファイルを読み込む
    const std::string file_path = asm_file_name(argv[1], "jinx");
    assert(!file_path.empty());
    std::ifstream script_file(file_path);

    //ランタイム環境を生成する
    auto first_runtime = Jinx::CreateRuntime();

    //ファイル内のテキスト領域
    Jinx::String script_txt = "";

    //テキストを読む
    while(!script_file.eof()){
        Jinx::String add_line = "";
        std::getline(script_file, add_line);
        script_txt += add_line + '\n';
    }

    //コンパイルする
    auto bytecode = first_runtime->Compile(script_txt.c_str(), argv[1], {Jinx::String("core")});
    //コンパイルの失敗を判断する
    if(!bytecode){
        //プログラムを終了する
        return 1;
    }

    //デバック情報を取り除く
    bytecode = first_runtime->StripDebugInfo(bytecode);

    //実行スクリプトを生成する
    auto script = runtime->CreateScript(bytecode);

    //script内に変数envを生成する
    //script->SetVariable("p", 123);

    //スクリプトを実行する
    do{
        //一時停止又は終了するまでスクリプトを実行する
        if(!script->Execute()){
            //スクリプトエラー
            return 1;
        }

        //外部変数の値を出力する
        //std::cout << "p: " << script->GetVariable("p").GetInteger() << std::endl;
    //スクリプトが終了したかを判断する
    }while(!script->IsFinished());

    //Jinxを終了させる
    //この関数は初期化以降に使用したオブジェクトを全て破棄した後に実行する。
    //プログラム終了時にも走っているらしい。
    //exit_jinx();

    return 0;
}