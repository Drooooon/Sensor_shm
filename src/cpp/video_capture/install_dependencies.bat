@echo off
echo 正在安装HTML转Word转换器所需的依赖包...
echo.

echo 安装 python-docx...
pip install python-docx

echo.
echo 安装 beautifulsoup4...
pip install beautifulsoup4

echo.
echo 安装 lxml...
pip install lxml

echo.
echo 依赖包安装完成！
echo.
echo 现在可以运行 html_to_word_converter.py 进行转换
pause
