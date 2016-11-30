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
      pro_file_path=$tests_path"/"$filename".pro"
      if test -e "$pro_file_path"; then

        # TODO : 이곳에 *.pro 파일을 해석한 내용을 입력한다

        echo "qmake $pro_file_path"
        `qmake $pro_file_path`
        `make`

        echo "./$filename"
        `./$filename`

        #for gcovname in `find $tests_path/. | egrep '\.cpp'`; do
        #  `gcov-5 -n -o $tests_path/. $gcovname > /dev/null`;
        #done

        for gcovname in $SOURCES; do
          `gcov-5 -n -o $tests_path/. $gcovname > /dev/null`;
        done
      else
        # TODO : 이곳에 *.pro 파일을 발견하지 못했다는 오류를 출력한다.
        echo "Not found file about $pro_file_path"
      fi
    fi
  fi
done


