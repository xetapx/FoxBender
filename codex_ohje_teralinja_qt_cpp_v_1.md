# FoxBender – Codex-ohje, osa 1: Projektin tavoite ja perusarkkitehtuuri

## Projektin nimi

FoxBender

## Tavoite

Rakenna Qt Creator / C++ -ohjelman prototyyppi terälinjan DXF-pohjaiseen CAM-suunnitteluun ja 2D-simulointiin.

Ohjelman pitää:

- avata DXF-tiedosto
- näyttää DXF väreillä ja tasoilla
- tunnistaa DXF-entityt
- antaa käyttäjän valita terälinja poluksi
- tallentaa valitut polut, ominaisuudet ja asetukset projektitiedostoon
- tuottaa looginen operaatio- ja ToolAction-lista
- simuloida työstö 2D:nä

Fyysistä koneohjausta ei tehdä ensimmäisessä vaiheessa.

---

# 1. Sovelluksen luonne

FoxBender on CAM-ohjelma, ei yleinen CAD-ohjelma.

Kevyt CAD-muokkaus sallitaan vain polun korjaamiseen:

- entityn pään raahaus toisen entityn päähän
- entityn katkaisu
- puuttuvan filletin teko kahden entityn väliin
- delete
- piirrä suora

Kaikki muu geometria tulee ensisijaisesti DXF:stä.

---

# 2. Teknologia

Käytä:

- C++17 tai uudempi
- Qt Widgets
- Qt Creator
- QGraphicsView / QGraphicsScene 2D-näkymään
- QDockWidget ominaisuuspaneeleihin
- QUndoStack undo/redo-toimintoihin
- JSON projektien ja asetusten tallennukseen

Ei vielä:

- PLC-ohjausta
- servo-/askelmoottoriohjausta
- reaaliaikaista IO:ta
- fyysistä koneohjausta

---

# 3. Pääikkuna

Pääikkunassa kolme päävälilehteä:

1. DXF / Polku
2. Asetukset
3. Simulointi

Lisäksi käytä dockable widgetejä:

- Layer panel
- Entity properties
- BladeLine properties
- Operation list
- Log / warnings

---

# 4. Totuuden lähde

Säilytä alkuperäinen DXF muuttumattomana.

Projektin totuuden lähde on:

- alkuperäinen DXF
- käyttäjän valitsemat BladeLine-polut
- käyttäjän lisäämät features/modifiers/layer-assignments

Älä tallenna vain lopullisia liikkeitä.

Operaatiot ja ToolAction-lista generoidaan tarvittaessa uudelleen DXF + projektiasetuksista.

---

# 5. Koordinaatisto ja yksiköt

Käytä DXF:n koordinaatistoa.

- 0,0 on vasen ala DXF-näkymän mukaan
- - X kasvaa oikealle ja Y kasvaa ylös
- näytä DXF sellaisena kuin se on
- kaikki mitat millimetreinä
- älä tue tuumia ensimmäisessä versiossa
- sisäinen laskenta double-arvoilla

---

# 6. Projektitiedostot ja asetukset

Tarvitaan kaksi tallennustasoa.

## Ohjelman yleiset asetukset

Tiedosto esimerkiksi:

```text
settings.json
```

Sisältää:

- työkalujen oletussijainnit
- työkalujen leveydet
- timeoutit
- oletustoleranssit
- taivutuskalibroinnit
- käyttöliittymäasetukset
- viimeksi avatut projektit

## Projektitiedosto

Tiedosto esimerkiksi:

```text
*.foxjob.json
```

Sisältää:

- projectVersion
- DXF-tiedoston polku
- BladeLine-polut
- path selection data
- layer assignments
- features
- modifiers
- toothing subpaths
- bridge settings
- simulation settings
- mahdolliset vanhat/tallennetut polut

Projektitiedostoon pitää aina lisätä:

```json
{
  "projectVersion": 1
}
```

Lisää autosave/recovery myöhemmin, mutta huomioi arkkitehtuurissa jo nyt.

---

# 7. Pääluokat

Tee selkeä jako UI:n, geometrian, simulaation ja myöhemmän koneohjauksen välille.

## UI

- MainWindow
- DxfViewWidget
- LayerPanel
- PropertyDockWidget
- SettingsWidget
- SimulationWidget
- LogWidget

## Geometry / Core

- GeometryModel
- DxfDocument
- DxfEntity
- PathFinder
- BladeLine
- PathSegment
- SegmentModifier
- LayerDefinition
- Feature

## Tools / Simulation

- ToolStation
- ToolAction
- SimulationModel
- SimulationScene
- ToolGraphicsItem

## Future Motion Layer

Varaa rajapinta myöhempää käyttöä varten:

- MachineAction
- MotionProgram
- MotionControllerInterface

Älä kytke simulaatiota suoraan IO:hon.

---

# 8. Työkalut ovat konfiguroitavia

Työkalut eivät saa olla kovakoodattu vain yhteen järjestykseen.

Nykyinen oletuskonfiguraatio annetaan asetuksissa, mutta myöhemmin pitää voida:

- lisätä työkalu
- poistaa työkalu
- vaihtaa työkalun sijaintia
- vaihtaa järjestystä
- vaihtaa leveyttä
- vaihtaa timeoutia

ToolStation sisältää vähintään:

```cpp
struct ToolStation {
    QString id;
    QString name;
    double centerOffsetMm;
    double widthMm;
    int actionTimeoutMs;
    bool enabled;
};
```

Kaikki työkalupositioarvot ovat työkalun keskikohtaan.

Leveillä työkaluilla pitää huomioida työkalun reuna alku- ja loppupään laskennassa.

---

# 9. Oletustyökalut ja sijainnit

0 mm = Optical Sensor.

| Offset | Tool | Width |
|---:|---|---:|
| -150 | Roughing Left | |
| -150 | Roughing Right | |
| -110 | Nicking | |
| -70 | Bridge 2.5 | 2.5 mm |
| -60 | Bridge 3.0 | 3.0 mm |
| -40 | Scraping | 6.0 mm |
| -30 | Notching | 6.0 mm |
| 0 | Optical Sensor | |
| 30 | Toothing 1.0 | 1.0 mm |
| 40 | Toothing 2.5 | 2.5 mm |
| 100 | Bending | |
| 160 | Cutting | 6.0 mm |

---

# 10. Simulaation periaate

Ensimmäinen simulointi on 2D.

- työkalut ovat paikallaan
- terälinja liikkuu
- työkalut ovat omia objektejaan
- työkalujen leveydet näytetään visuaalisesti
- feed-position ja ToolAction-tapahtumat näytetään
- timeout näkyy syötön pysähtymisenä

Tee toteutus niin, että myöhemmin voidaan lisätä 3D-simulointi.

Simulaatio saa olla jatkuva animaatio, mutta mukana pitää olla myös askellus tapahtuma kerrallaan.

---

# 11. Syöttösääntö

Normaali tuotantoajo saa liikuttaa linjaa vain eteenpäin.

Poikkeus:

- aloitussekvenssi
- siinä voidaan tehdä scraping linjan alkuun
- sen jälkeen voidaan tehdä muita alkuoperaatioita, esimerkiksi nicking

Aloitussekvenssin jälkeen ToolAction-listan feedPositionMm pitää olla kasvava.

Jos jokin operaatio vaatisi taaksepäin siirtoa aloitussekvenssin jälkeen, simulaation pitää antaa varoitus tai virhe.

---

# 12. Operaatiot ovat blocking

Ensimmäisessä versiossa kaikki operaatiot ovat blocking.

Eli:

- syöttö pysähtyy
- työkalu tekee työvaiheen
- odotetaan actionTimeoutMs
- syöttö jatkuu

Ei vielä pipeline-ajoa.

---

# 13. Undo / Redo

Käytä QUndoStackia ainakin näille:

- polun valinnat
- features
- modifiers
- layer-assignments
- kevyt geometriaeditointi

---

# 14. Validointi ja logitus

Tee heti logiikka:

- operation log
- simulation log
- geometry warnings
- feature conflict warnings
- tool scheduling warnings

UI:ssa pitää olla Log / Warnings -paneeli.

---

# 15. Testidata

Tarvitaan pienet testitapaukset:

- suora linja
- yksi kaari
- kulma
- bridge/portti
- toothing/hammastus
- nicking
- monimutkaisempi DXF useilla layereilla

---

# 16. Seuraavat ohjeen osat

Tämä oli osa 1.

Seuraavat osat kannattaa tehdä erillisinä:

- Osa 2: DXF, geometriamalli ja polunvalinta
- Osa 3: Features, modifiers ja layer assignments
- Osa 4: Työkaluasemat, ToolAction ja 2D-simulointi
- Osa 5: UI-ikkunat ja käyttäjätoiminnot
- Osa 6: JSON-projekti- ja asetustiedostot
- Osa 7: myöhempi motion-control-rajapinta

