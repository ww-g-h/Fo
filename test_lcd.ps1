# ESP32-S3 LCD测试脚本
# 通过串口自动发送测试命令

param(
    [string]$Port = "COM3",
    [int]$BaudRate = 115200
)

Write-Host "=== ESP32-S3 LCD 测试脚本 ===" -ForegroundColor Green
Write-Host "串口: $Port" -ForegroundColor Cyan
Write-Host "波特率: $BaudRate" -ForegroundColor Cyan
Write-Host ""

# 检查串口是否可用
try {
    $serial = New-Object System.IO.Ports.SerialPort
    $serial.PortName = $Port
    $serial.BaudRate = $BaudRate
    $serial.DataBits = 8
    $serial.Parity = [System.IO.Ports.Parity]::None
    $serial.StopBits = [System.IO.Ports.StopBits]::One
    $serial.Handshake = [System.IO.Ports.Handshake]::None
    $serial.ReadTimeout = 500
    $serial.WriteTimeout = 500
    
    $serial.Open()
    Write-Host "串口打开成功!" -ForegroundColor Green
    Start-Sleep -Seconds 2
    
    # 测试命令列表
    $testCases = @(
        @{No=1; Cmd="hello"; Desc="显示 Hello World!"},
        @{No=2; Cmd="wave"; Desc="显示挥手图标"},
        @{No=3; Cmd="smile"; Desc="显示笑脸图标"},
        @{No=4; Cmd="sad"; Desc="显示哭脸图标"},
        @{No=5; Cmd="clear"; Desc="清空屏幕"},
        @{No=6; Cmd="hello"; Desc="连续测试 - hello"},
        @{No=6; Cmd="smile"; Desc="连续测试 - smile"},
        @{No=6; Cmd="wave"; Desc="连续测试 - wave"},
        @{No=7; Cmd="unknown"; Desc="测试无效命令"}
    )
    
    Write-Host ""
    Write-Host "开始执行测试..." -ForegroundColor Yellow
    Write-Host "=" * 60 -ForegroundColor Gray
    
    foreach ($test in $testCases) {
        Write-Host ""
        Write-Host "[测试 $($test.No)] $($test.Desc)" -ForegroundColor Cyan
        Write-Host "发送命令: $($test.Cmd)" -ForegroundColor White
        
        # 发送命令
        $serial.WriteLine($test.Cmd)
        
        # 等待显示
        Start-Sleep -Seconds 3
        
        Write-Host "✓ 完成" -ForegroundColor Green
    }
    
    Write-Host ""
    Write-Host "=" * 60 -ForegroundColor Gray
    Write-Host "所有测试完成!" -ForegroundColor Green
    Write-Host ""
    
    # 交互模式
    Write-Host "进入交互模式（输入 'exit' 退出）：" -ForegroundColor Yellow
    while ($true) {
        $input = Read-Host "命令"
        if ($input -eq "exit") {
            break
        }
        if ($input) {
            $serial.WriteLine($input)
            Write-Host "已发送: $input" -ForegroundColor Green
        }
    }
    
    $serial.Close()
    Write-Host "串口已关闭" -ForegroundColor Yellow
}
catch {
    Write-Host "错误: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "请检查:" -ForegroundColor Yellow
    Write-Host "1. 串口号是否正确 (默认 COM3)" -ForegroundColor White
    Write-Host "2. ESP32-S3 是否已连接" -ForegroundColor White
    Write-Host "3. 串口是否被其他程序占用" -ForegroundColor White
    Write-Host ""
    Write-Host "使用方法: .\test_lcd.ps1 -Port COM端口号" -ForegroundColor Cyan
    Write-Host "示例: .\test_lcd.ps1 -Port COM5" -ForegroundColor Cyan
}
finally {
    if ($serial -and $serial.IsOpen) {
        $serial.Close()
    }
}
