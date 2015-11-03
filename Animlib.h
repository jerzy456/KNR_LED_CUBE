#ifndef Animlib_H
#define AnimLib_H

void Deszcz (int liczba_iteracji);
void Plaszczyzna(int liczba_iteracji);
void Miganie();
void MiganieZTranslatorem(int liczba_iteracji);
void WoopWoop(int liczba_iteracji);
void HoopHoop(int liczba_iteracji);
void WoopSelektor(uint8_t numerWoop, uint8_t stanWoop);
void HoopSelektor(uint8_t numerHoop, uint8_t stanHoop);
void Czysc(); //zeruje rejestry wyswietlacza

#endif