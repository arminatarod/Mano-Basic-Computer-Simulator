[System.Reflection.Assembly]::LoadWithPartialName("System.windows.forms") | out-null

$OpenFileDialog = New-Object System.Windows.Forms.OpenFileDialog
$OpenFileDialog.initialDirectory = $initialDirectory
$OpenFileDialog.filter = "Text file (*.txt)|*.txt"
$OpenFileDialog.ShowDialog()
$OpenFileDialog.filename