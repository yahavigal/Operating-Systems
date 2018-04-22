mkdir 200921740
cd 200921740
mkdir temp
cd temp
echo yahav > yahav
echo avigal > avigal
echo yahavavigal > yahavavigal
cp yahav ../avigal
cp avigal ../yahav
rm yahav
rm avigal
mv yahavavigal ..
cd ..
rmdir temp
echo "The content of the directory:"
ls -l -a
echo "The content of yahav:"
cat yahav
echo "The content of avigal:"
cat avigal
echo "The content of yahavavigal:"
cat yahavavigal

