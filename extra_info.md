## 1) Convenția `codCol` (moduri de randare)

Shaderul principal folosește `codCol` ca un „switch” logic:

- `codCol == 0` – obiecte iluminate + umbre (teren, copaci)
- `codCol == 2` – obiecte „unlit” (cer, nori, stele, UI)
- `codCol == 3` – unlit cu `overrideColor` (licurici)
- `codCol == 4` – soare/lună (unlit)
- `codCol == 6` – flacără flipbook (textură animată)
- `codCol == 7` – obiecte texturate simple + lumină (campfire, bănci)
- `codCol == 8` – glow procedural (aura focului / pata de lumină)

---

## 2) Teren procedural (dealuri): funcția de înălțime + normale

### 2.1) Funcția de înălțime `y = h(x,z)`

Terenul este definit prin:

`h(x,z) = 2·sin(0.03x) + 1.5·cos(0.025z) + 0.8·sin(0.02(x+z))`

Interpretare:
- fiecare termen este un „val” pe o axă (sau diagonală)
- amplitudinile (2, 1.5, 0.8) controlează cât de „înalt” e dealul
- frecvențele (0.03, 0.025, 0.02) controlează cât de dese sunt undele (dealuri dese vs rare)

### 2.2) Mesh-ul terenului (grid)

Se creează un grid uniform de (gridSize+1)² vertecși (ex. 201×201):
- `x,z` sunt eșantionate uniform pe un pătrat mare
- `y` se calculează din `h(x,z)`

Indicele este făcut din triunghiuri:
- fiecare celulă (quad) devine 2 triunghiuri
- topologie constantă → randare eficientă cu EBO

### 2.3) Normala terenului

În loc să derivăm analitic `∂h/∂x` și `∂h/∂z`, se folosește aproximație numerică:

- `hL = h(x-ε, z)`
- `hR = h(x+ε, z)`
- `hD = h(x, z-ε)`
- `hU = h(x, z+ε)`

Vector „pseudo-normală”:

`n ≈ normalize( (hL - hR, 2ε, hD - hU) )`

Intuiție:
- componenta X ~ panta pe X
- componenta Z ~ panta pe Z
- componenta Y e „ridicată” cu `2ε` ca să nu devină normală prea orizontală / instabilă

### 2.4) Culoarea terenului în funcție de înălțime (gradient)

Se calculează un parametru `t` din `y`:

`t = clamp( (y + 6) / 12, 0..1 )`

Apoi se interpolează între două nuanțe de verde:

`color = mix(greenDark, greenLight, t)`

Asta creează variații subtile (dealurile mai înalte ușor mai deschise).

---

## 3) Iluminare (model simplu) + coordonate folosite

### 3.1) Direcția luminii

Lumina este tratată ca „soare/lună” (o lumină globală):

`L = normalize(lightPos - fragPos)`

Chiar dacă „soarele” e departe, în practică e suficient pentru efect.

### 3.2) Componente de iluminare

Pe obiectele `codCol==0` (în vertex shader) și `codCol==7` (în fragment shader) se folosește:

- Ambient: `ambient = k_a * baseColor`
- Difuz: `diffuse = max(dot(N, L), 0) * baseColor * lightColor`
- Specular: `specular = k_s * pow(max(dot(N, H), 0), shininess)` unde `H = normalize(L + V)`

`V = normalize(viewPos - fragPos)`.

---

## 4) Umbre (Shadow Mapping)

Shadow mapping are 2 pași:

### 4.1) Pasul 1: randarea hărții de umbre (depth-only)

Se construiește o matrice „light space”:

`lightSpaceMatrix = lightProj * lightView`

- `lightView = lookAt(lightCamPos, lightTarget, up)`
- `lightProj = ortho(-S, S, -S, S, near, far)`

Se randează scena cu un shader care scrie doar adâncimea în `ShadowDepthTex`.

### 4.2) Pasul 2: testul de umbră în shaderul principal

În fragment shader:
1. transformăm punctul în light space:
   `pLS = lightSpaceMatrix * vec4(fragPos, 1)`
2. facem perspectives divide:
   `proj = pLS.xyz / pLS.w`
3. mapăm din [-1..1] în [0..1]:
   `uvz = proj*0.5 + 0.5`
4. comparăm `uvz.z` cu valoarea din `shadowMap(uvz.xy)`

Dacă `uvz.z` e mai mare decât depth-ul din hartă → fragmentul e în umbră.

### 4.3) PCF (Percentage Closer Filtering)

Ca să nu fie umbrele „dinți”, se face o medie pe mai multe sample-uri în jurul texelului:
- aici este un PCF 2×2

`shadow = average( step(depthTex, currentDepth - bias) )`

Bias-ul reduce „shadow acne” și depinde de unghi:

`bias = max(0.0012*(1 - dot(N,L)), 0.0004)`

---

## 5) Cer (Skybox) + gradient

Skybox-ul este un cub mare, desenat fără `GL_DEPTH_TEST`.

Gradientul este „geometric”: vârfurile de sus au o culoare, cele de jos alta.
GPU interpolează culoarea pe triunghiuri → rezultă gradientul.

Schimbarea zi/apus/noapte:
- se schimbă `topColor` și `bottomColor`
- se rescrie VBO cu noile culori

---

## 6) Nori: distribuție spațială + mișcare

### 6.1) Distribuție

Norii sunt plasați cu random uniform în XZ:
- `x ∈ [-400, 400]`
- `z ∈ [-400, 400]`

Înălțimea:
- `y ∈ [100, 180]`


### 6.2) Scalare

Fiecare nor e un cub scalat:
- `scaleX` mare, `scaleY` mic, `scaleZ` mediu

### 6.3) Mișcare (vânt)

Poziția se translatează pe X:

`x(t) = x0 + v*t` (aici `v` este o constantă mică)

Când depășește limita, se face wrap-around (loop pe hartă).

---

## 7) Soare/Lună: billboarding

Billboarding înseamnă: obiectul (un disc) rămâne mereu orientat spre cameră.

Trucul folosit:
- matricea `model` conțin translația la poziția soarelui
- partea de rotație a lui `model` este înlocuită cu rotația camerei (derivată din `view`)

Practic, luăm axele camerei și le copiem în `model`, astfel încât planul discului să fie paralel cu ecranul.

---

## 8) Foc: flipbook + lumină punctiformă

### 8.1) Flipbook (animare 2D)

Avem N texturi (cadre). Cadrul curent este:

`frame = floor(time * fireFps) mod N`

Flacăra este un „plane” randat ca billboard.

### 8.2) Lumină de foc (atenuare)

Contribuția focului scade cu distanța:

`att = 1 / (1 + k1*d + k2*d^2)`

Apoi:

`fireDiffuse = max(dot(N, fireDir), 0) * fireColor * att`

---

## 9) Glow (aură / pata de lumină): gradient radial

În shader:
- se calculează distanța față de centrul UV-ului: `dist = distance(uv, 0.5)`
- `alpha = 1 - smoothstep(0, 0.5, dist)`
- `alpha = alpha^3` pentru un centru mai intens

Se folosește blending aditiv ca să pară lumină.

---

## 10) Licurici: mișcare sinusoidală + clipire + lumină

### 10.1) Mișcare 

Pentru fiecare particulă:
- `offsetX = sin(t*speed + offset.x) * A`
- `offsetZ = cos(t*speed*0.8 + offset.z) * A`
- `offsetY = sin(t*speed*0.5 + phase) * yRange`

Poziția:

`pos(t) = center + offset + (offsetX, offsetY, offsetZ)`

Aceste frecvențe diferite (1.0, 0.8, 0.5) creează o mișcare care nu se repetă rapid

### 10.2) Clipire

`blink = pow( (sin(3t + phase)+1)/2 , 4 )`

Motiv:
- `sin(...)` produce o oscilație periodică.
- `(+1)/2` mapează intervalul din `[-1..1]` în `[0..1]`.
- `pow(x,4)` comprimă semnalul: stă mult timp aproape de 0 și are „vârfuri” scurte aproape de 1 → pare clipire (flash).

### 10.3) Licuricii ca lumini punctiforme (iluminare în shader)

În proiect, licuricii au două roluri:

1) **Particule vizibile** (sprite/billboard): quad transparent care se vede în aer.
2) **Surse de lumină** (foarte slabe): influențează ușor culoarea terenului/obiectelor din jur.


În fragment shader există:
- `uniform vec3 fireflyPos[MAX_FIREFLY_LIGHTS];`
- `uniform int numFirefliesActive;`

#### 10.3.2) Calculul distanței și direcției
Pentru fiecare fragment (pixel) al terenului/obiectelor:
- `d = length(fragPos - fireflyPos[i])`

Dacă `d` e mare, efectul trebuie să fie aproape 0.

#### 10.3.3) Atenuarea luminii (de ce forma `1/(1 + a·d + b·d²)`)
Se folosește o formulă standard de atenuare pentru lumina punctiformă:

`ffAtt = 1 / (1 + a*d + b*d*d)`

Intuiție:
- termenul `1` (constant) previne valori infinite la `d=0`
- termenul `a*d` controlează căderea „aproape” (linear falloff)
- termenul `b*d^2` controlează căderea rapidă la distanțe mai mari (quadratic falloff)

Matematic, `d^2` face ca efectul să se stingă mult mai repede → util pentru licurici, care trebuie să lumineze doar pe o rază mică.

În shader valorile `a` și `b` sunt setate relativ mari (ex. `a=2`, `b=5`) ca lumina să fie foarte locală.

#### 10.3.4) Culoarea și acumularea luminii
Fiecare licurici contribuie cu o culoare verde-gălbuie:

`ffColor = (0.4, 0.9, 0.1)`

Contribuția per licurici:

`ffLighting += ffColor * ffAtt * intensity`

unde `intensity` este un factor de reglaj (ex. `1.5`).

Apoi se adună în iluminarea totală:

`finalLighting = baseLighting + ffLighting`


#### 10.3.5) Legătura cu  blink
Selecția pozițiilor trimise la shader se face astfel încât să includă doar licuricii care sunt „aprinsi” (blink peste un prag).
Asta înseamnă:
- când licuriciul e „stins”, nu contribuie cu lumină
- când are un spike de blink, apare și lumina lui punctuală

Rezultatul vizual: un „flicker” difuz pe iarbă/obiecte când licuricii trec și se aprind.

---

## 11) Ceață + volumetric (god rays)

### 11.1) Ceață clasică (linear fog)

`f = clamp( (fogEnd - dist) / (fogEnd - fogStart), 0..1 )`

`final = mix(fogColor, shadedColor, f)`

### 11.2) God rays (aproximare)

Se eșantionează pe segmentul cameră → fragment.
Pentru fiecare punct:
- se verifică dacă e în lumină (comparare cu shadowMap)
- se acumulează `inLight`

Rezultatul se adaugă ca „lumină în aer” (scattering aproximat).

---

## 12) Bănci: plasare pe cerc + interacțiune (așezare / ridicare)

### 12.1) Plasarea băncilor pe un cerc (geometrie în plan XZ)

Băncile sunt poziționate în jurul focului pe un cerc, în planul XZ.

1) Se pornește de la poziția focului în lume:
- `CampfirePos = (CAMPFIRE_SCENE_POS.x, GroundHeight(x,z), CAMPFIRE_SCENE_POS.z)`

2) Se definește centrul cercului băncilor ca o translație față de foc:
- `SeatCircleCenter = CampfirePos + seatCenterOffset`

3) Pentru `seatCount` bănci, unghiul fiecărei bănci este:

`a_i = i * (2π / seatCount) + a0`

unde `a0` este un offset (ex. `0.785`) ca să nu fie aliniate „perfect” pe axe.

4) Poziția în plan XZ:

`p_i = SeatCircleCenter + (cos(a_i) * seatRadius, 0, sin(a_i) * seatRadius)`

5) Înălțimea (Y) se aliniază pe teren:

`p_i.y = GroundHeight(p_i.x, p_i.z) + yOffset`

`yOffset` este un mic offset ca banca să nu intre în sol.

### 12.2) Orientarea băncii: să „privească” spre centru

Ca banca să fie orientată spre foc (centru), se calculează un yaw (rotație în jurul axei Y):

`yawToCenter = atan2(SeatCircleCenter.x - p_i.x, SeatCircleCenter.z - p_i.z)`

Apoi se transformă în grade și se adaugă o rotație manuală (în funcție de cum e exportat modelul OBJ):

`finalYawDeg = degrees(yawToCenter) + manualRotations[i]`

Această rotație manuală există pentru că „forward direction” al modelului `.obj` poate să nu fie aliniată cu axele așteptate.

### 12.3) Detectarea proximității

În fiecare frame se verifică distanța camerei față de fiecare bancă:

`dist = length(cameraPos - p_i)`

Dacă `dist < interactRadius` (ex. `10.0f`), atunci:
- `g_canInteract = true`
- `g_potentialSitPos = p_i` (poziția către care vom muta camera ca să „stăm”)

### 12.4) Așezarea (tasta `E`)

Când apeși `E` și suntem în range:
- `g_isSitting = true`
- se salvează poziția curentă: `g_restorePos = camera.Position`
- pornește tranziția: `g_transition.active = true`

Ținta tranziției (poziția finală a camerei) este:

`endPos = g_potentialSitPos + (0, eyeHeight, 0)`

unde `eyeHeight` (ex. `2.5f`) ridică camera ca să simuleze înălțimea ochilor.
- în timpul tranziției se interpolează doar poziția (LERP)
- direcția camerei (`Front`) rămâne constantă (se păstrează direcția existentă la apăsarea tastei)

### 12.5) Interpolare 

Pe durata tranziției (ex. `1.2s`):

- `Position(t) = mix(startPos, endPos, smoothT)`

`smoothT` este un easing de tip smoothstep:

`smoothT = t^2 * (3 - 2t)`

Orientarea:
- `Front` rămâne fix (valoarea salvată la începutul tranziției)
- din `Front` se recalculează `Right` și `Up` cu produs vectorial

### 12.6) Ridicarea (tasta `Space`)

Când ești așezat și apeși `Space`:
- `g_isSitting = false`
- pornește o tranziție simplă înapoi la `g_restorePos` (plus un mic offset pe Y)
- direcția (`Front`) rămâne constantă pe durata tranziției

---


