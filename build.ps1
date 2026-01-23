# ESP32-S3 项目构建和烧录脚本

param(
    [string]$Action = "build",
    [string]$Port = "COM3"
)

$ProjectPath = "E:\Fo\Fo"

Write-Host "=== ESP32-S3 项目构建工具 ===" -ForegroundColor Green
Write-Host "项目路径: $ProjectPath" -ForegroundColor Cyan
Write-Host "操作: $Action" -ForegroundColor Cyan
Write-Host ""

# 切换到项目目录
Set-Location $ProjectPath

switch ($Action.ToLower()) {
    "build" {
        Write-Host "开始编译项目..." -ForegroundColor Yellow
        idf.py build
    }
    
    "flash" {
        Write-Host "烧录到 $Port..." -ForegroundColor Yellow
        idf.py -p $Port flash
    }
    
    "monitor" {
        Write-Host "打开串口监视器 ($Port)..." -ForegroundColor Yellow
        idf.py -p $Port monitor
    }
    
    "all" {
        Write-Host "编译并烧录..." -ForegroundColor Yellow
        idf.py build
        if ($LASTEXITCODE -eq 0) {
            Write-Host "编译成功，开始烧录..." -ForegroundColor Green
            idf.py -p $Port flash monitor
        } else {
            Write-Host "编译失败!" -ForegroundColor Red
        }
    }
    
    "clean" {
        Write-Host "清理项目..." -ForegroundColor Yellow
        idf.py fullclean
    }
    
    "menuconfig" {
        Write-Host "打开配置菜单..." -ForegroundColor Yellow
        idf.py menuconfig
    }
    
    default {
        Write-Host "未知操作: $Action" -ForegroundColor Red
        Write-Host ""
        Write-Host "可用操作:" -ForegroundColor Yellow
        Write-Host "  build       - 仅编译项目" -ForegroundColor White
        Write-Host "  flash       - 仅烧录（需要先编译）" -ForegroundColor White
        Write-Host "  monitor     - 打开串口监视器" -ForegroundColor White
        Write-Host "  all         - 编译、烧录并打开监视器" -ForegroundColor White
        Write-Host "  clean       - 清理编译文件" -ForegroundColor White
        Write-Host "  menuconfig  - 打开配置菜单" -ForegroundColor White
        Write-Host ""
        Write-Host "使用示例:" -ForegroundColor Cyan
        Write-Host "  .\build.ps1 -Action build" -ForegroundColor White
        Write-Host "  .\build.ps1 -Action flash -Port COM5" -ForegroundColor White
        Write-Host "  .\build.ps1 -Action all -Port COM5" -ForegroundColor White
    }
}
