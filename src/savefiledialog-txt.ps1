[System.Reflection.Assembly]::LoadWithPartialName("System.windows.forms") | out-null

$SaveFileDialog = New-Object System.Windows.Forms.SaveFileDialog
$SaveFileDialog.initialDirectory = $initialDirectory
$SaveFileDialog.filter = "Text file (*.txt)|*.txt"
$SaveFileDialog.CheckPathExists = $true
$SaveFileDialog.CreatePrompt = $true
$SaveFileDialog.OverwritePrompt = $true
$SaveFileDialog.ShowDialog()
$SaveFileDialog.FileName