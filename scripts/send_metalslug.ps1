param (
    [Parameter(Mandatory=$true)]
    [string]$PortName,
    [string]$FilePath = "build_arm\metalslug",
    [int]$BaudRate = 115200
)

Write-Host "Iniciando transferência para $PortName..."
$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, 'None', 8, 'One'
$port.WriteBufferSize = 8192

try {
    $port.Open()
} catch {
    Write-Host "ERRO: Não foi possível abrir a porta $PortName. O Putty está aberto? Feche o Putty primeiro!"
    exit 1
}

Write-Host "Porta aberta. Preparando o Linux para receber o arquivo..."
$port.Write("
")
Start-Sleep -Seconds 1
$port.Write("stty -echo
")
Start-Sleep -Seconds 1
$port.Write("base64 -d > /root/metalslug
")
Start-Sleep -Seconds 1

Write-Host "Lendo arquivo $FilePath..."
$bytes = [System.IO.File]::ReadAllBytes($FilePath)
$b64 = [Convert]::ToBase64String($bytes)

Write-Host "Tamanho em Base64: $($b64.Length) bytes."
Write-Host "Enviando arquivo em blocos..."

$chunkSize = 1024
$totalSent = 0

for ($i = 0; $i -lt $b64.Length; $i += $chunkSize) {
    $len = [Math]::Min($chunkSize, $b64.Length - $i)
    $port.Write($b64.Substring($i, $len))
    Start-Sleep -Milliseconds 15
    $totalSent += $len
    if ($totalSent % ($chunkSize * 100) -eq 0) {
        $percent = [Math]::Round(($totalSent / $b64.Length) * 100)
        Write-Host "$percent% enviado..."
    }
}

Write-Host "100% enviado! Finalizando arquivo..."
Start-Sleep -Seconds 1
$port.Write("
")
Start-Sleep -Seconds 1
$port.Write("x04")
Start-Sleep -Seconds 1
$port.Write("chmod +x /root/metalslug
")
Start-Sleep -Seconds 1
$port.Write("stty echo
")
Start-Sleep -Seconds 1
$port.Close()
Write-Host "Transferência concluída com sucesso! Pode abrir o Putty novamente."
