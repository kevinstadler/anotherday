<?php
header('Content-Type:text/plain');
header('Access-Control-Allow-Origin: *');

$file = fopen('lux.txt', 'r');
fseek($file, -80000, SEEK_END);

$before = intval($_GET['before']) / 1000;
$cur = $before; // run through loop at least once

while (true) {
  # seek to end of line
  if (fgets($file) == false) {
    break;
  }
  # seek to first timestamped line
  while (($line = fgets($file))[0] == "\t") { }

  if ($cur < $before) {
    break;
  }
  $cur = intval($line);
//  echo $cur . "\n";

  if (fseek($file, -160000, SEEK_CUR) != 0) {
    break;
  }
}

fseek($file, -strlen($line), SEEK_CUR);

echo "ts,bt,lux,cct,c,r,g,b,v,temp,hum,fails\n";
while (($data = fgetcsv($file, 1000, "\t")) !== FALSE) {
  if ($data[0] != '') {
    $ts = intval($data[0]);
  } else if ($data[1] == '0') {
    # some of the formerly invalid non-registered initial boot uploads
    continue;
  }
  $now = $ts + intval($data[2]);
  if ($now >= $before) {
    break;
  }
  $cpl = 2.4 * $data[4] * $data[3] / 310;
  $ir = ($data[6] + $data[7] + $data[8] - $data[5] ) / 2;
  $gbb = .136 * ($data[6] - $ir) + $data[7] - $ir - .444 * ($data[8] - $ir);
  $lux = max(.01, round($gbb / $cpl, 1));
  $cct = round(1391 + 3810 * ($data[8] - $ir) / ($data[6] - $ir));

  echo $now . '000,' . $data[1] .',' . $lux .',' . $cct;
  #crgb, v, temp, hum, fails
  for ($i = 5; $i < 13; $i++) {
    echo ',' . $data[$i];
  }

  echo "\n";
}


?>