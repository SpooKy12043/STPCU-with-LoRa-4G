function Decoder(bytes, port) {
// Se define el tamaño del payload
  if (bytes.length === 15) {
     // Identificar los datos de la silla y el ID del Bus en el payload 
    const BUS_ID = bytes[0];
    const iSilla = bytes[1];
    const ZX     = bytes[2];
    const Oc     = bytes[3];

    // Identificar los datos del GPS y Conteo en el payload
    const latInt = (bytes[5] << 8) | bytes[4];
    const lonInt = (bytes[7] << 8) | bytes[6];
    const altInt = (bytes[11] << 24) | (bytes[10] << 16) | (bytes[9] << 8) | bytes[8];
    const speedInt = (bytes[13] << 8) | bytes[12];
    const personCount = bytes[14];

    // Corrección de valores
    const lat = latInt >= 32768 ? latInt - 65536 : latInt;
    const lon = lonInt >= 32768 ? lonInt - 65536 : lonInt;
    const alt = altInt & 0x80000000 ? altInt - 0x100000000 : altInt;
    const speed = speedInt >= 32768 ? speedInt - 65536 : speedInt;

    return {
      BUS_ID: BUS_ID,
      iSilla: iSilla,
      ZX: ZX,
      Oc: Oc,
      Lat: lat / 100,
      Lon: lon / 100,
      Alt: alt / 100,
      Speed: speed / 100,
      Pasajeros: personCount
    };
  } else {
    return { error: "Invalid payload length: " + bytes.length };
  }
}
