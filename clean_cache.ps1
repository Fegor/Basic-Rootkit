Write-Host "正在清理 Visual Studio 缓存..." -ForegroundColor Yellow

# 递归删除 .vs 文件夹
Get-ChildItem -Path . -Include .vs -Recurse -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force

# 递归删除所有 .pch 和 .ipch 文件
Get-ChildItem -Path . -Include *.pch, *.ipch -Recurse -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force

Write-Host "清理完毕。" -ForegroundColor Green