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

        # build and execute
        build_command="g++ -fprofile-arcs -ftest-coverage -o $tests_path/tests ${HEADERS[@]} ${SOURCES[@]}"
        build_command="$build_command $tests_path/test_shell.cpp $tests_path/$x -lgcov -lpthread"
        exec_command="$tests_path/tests"

        echo $build_command
        echo $exec_command
        `$build_command`
        `$exec_command &> /dev/null`

        for gcovname in ${!SOURCES[*]}; do
          echo "gcov -n -o . $gcovname > /dev/null"
          `gcov -n -o . $gcovname > /dev/null`;
        done

        result=$?
        if [ "$result" != "0" ]; then
          echo "$x($result): exit $result"
          exit $result
        fi
      else
        # TODO : 이곳에 *.dep 파일을 발견하지 못했다는 오류를 출력한다.
        echo "Not found file about $dep_file_path"
      fi
    fi
  fi
done


