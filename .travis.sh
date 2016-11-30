tests_path="`pwd`/tests"
# get all test cpp files
compile_list=`ls -l $tests_path | grep ^- | awk '{split($0,arr," "); print arr[9];}'`
for x in $compile_list; do
  # check test-shell source files
  if [ "$x" != "test_shell.cpp" ]; then

    is_source_file=`echo $x | grep -E "(cpp|c)$"`
    if test -n "$is_source_file"; then

      # check to execute about *.dep file
      filename=`echo $x | awk '{split($0,arr,"."); print arr[1];}'`
      dep_file_path=$tests_path"/"$filename".dep"
      if test -e "$dep_file_path"; then

        # TODO : 이곳에 *.dep 파일을 해석한 내용을 입력한다
        source $dep_file_path

        for i in ${!HEADERS[*]}; do
          HEADERS[$i]="-I"$tests_path"/"${HEADERS[$i]}
        done
        for i in ${!SOURCES[*]}; do
          SOURCES[$i]=$tests_path"/"${SOURCES[$i]}
        done

        SOURCES += ${SOURCES[@]} $tests_path/test_shell.cpp $tests_path/$x

        QMAKE_CXXFLAGS += -Wall -Wextra -Weffc++ -Werror

        # gcov
        QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
        LIBS += -lgcov -lpthread

        # C++11
        QMAKE_CXX = g++-5
        QMAKE_LINK = g++-5
        QMAKE_CC = gcc-5
        QMAKE_CXXFLAGS += -std=c++11

        `qmake travis_qmake_gcc_cpp11_gcov.pro`
        `make`

        `./travis_qmake_gcc_cpp11_gcov`

        for filename in `find $tests_path/. | egrep '\.cpp'`; do
          gcov-5 -n -o $tests_path/. $filename > /dev/null;
        done
        for filename in ${!SOURCES[*]}; do
          gcov-5 -n -o $tests_path/. $filename > /dev/null;
        done

        # build and execute
        #build_command="g++ -o $tests_path/tests ${HEADERS[@]} ${SOURCES[@]}"
        #build_command="$build_command $tests_path/test_shell.cpp $tests_path/$x -lpthread"
        #exec_command="$tests_path/tests"

        #echo $build_command
        #echo $exec_command
        #`$build_command`
        #`$exec_command &> /dev/null`

        #result=$?
        #if [ "$result" != "0" ]; then
        #  echo "$x($result): exit $result"
        #  exit $result
        #fi
      else
        # TODO : 이곳에 *.dep 파일을 발견하지 못했다는 오류를 출력한다.
        echo "Not found file about $dep_file_path"
      fi
    fi
  fi
done


