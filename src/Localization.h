#pragma once
#include <string>
#include <string_view>
#include <array>
#include "Rewards.h"
namespace an {
// Order of labels is the public reward enum, not alphabetical order.
struct Locale {
 const wchar_t *code,*aliases,*font;
 std::array<const wchar_t*,Count> labels;
 const wchar_t *notice,*installed,*available,*instructions,*close,*open;
};
inline constexpr Locale locales[]={
 {L"en",L"en|en-us|english|english us|english (us)|us",L"Segoe UI",
  {L"Near miss",L"Oncoming lane",L"Oncoming near miss",L"Jump",L"Airtime",L"Drift",L"Slipstream",L"Near-miss chain",L"Top speed",L"Drift streak"},
  L"A newer version is available.",L"Installed",L"Available",L"Update after closing the game.\r\nNo automatic download or installation.",L"Close and continue",L"Open GitHub"},
 {L"en-GB",L"en-gb|en-uk|english uk|english (uk)|uk",L"Segoe UI",
  {L"Near miss",L"Oncoming lane",L"Oncoming near miss",L"Jump",L"Airtime",L"Drift",L"Slipstream",L"Near-miss chain",L"Top speed",L"Drift streak"},
  L"A newer version is available.",L"Installed",L"Available",L"Update after closing the game.\r\nNo automatic download or installation.",L"Close and continue",L"Open GitHub"},
 {L"fr",L"fr|fr-fr|french",L"Segoe UI",
  {L"Frôlement",L"Contresens",L"Frôlement à contresens",L"Saut",L"Temps en l’air",L"Dérapage",L"Aspiration",L"Frôlements en série",L"Vitesse de pointe",L"Dérapage prolongé"},
  L"Une nouvelle version est disponible.",L"Version installée",L"Version disponible",L"Fermez le jeu avant la mise à jour.\r\nAucun téléchargement ni installation automatique.",L"Fermer et continuer",L"Ouvrir GitHub"},
 {L"de",L"de|de-de|german",L"Segoe UI",
  {L"Beinaheunfall",L"Gegenfahrbahn",L"Beinaheunfall im Gegenverkehr",L"Sprung",L"Flugzeit",L"Drift",L"Windschatten",L"Beinaheunfall-Serie",L"Höchstgeschwindigkeit",L"Dauerdrift"},
  L"Eine neue Version ist verfügbar.",L"Installiert",L"Verfügbar",L"Vor dem Aktualisieren das Spiel schließen.\r\nKein automatischer Download oder Installation.",L"Schließen und fortfahren",L"GitHub öffnen"},
 {L"it",L"it|it-it|italian",L"Segoe UI",
  {L"Collisione sfiorata",L"Contromano",L"Rischio contromano",L"Salto",L"Tempo in aria",L"Derapata",L"Scia",L"Rischi consecutivi",L"Velocità massima",L"Derapata prolungata"},
  L"È disponibile una nuova versione.",L"Installata",L"Disponibile",L"Chiudi il gioco prima di aggiornare.\r\nNessun download o installazione automatica.",L"Chiudi e continua",L"Apri GitHub"},
 {L"es",L"es|es-es|spanish",L"Segoe UI",
  {L"Roce",L"Sentido contrario",L"Roce en sentido contrario",L"Salto",L"Tiempo en el aire",L"Derrape",L"Rebufo",L"Roces consecutivos",L"Velocidad máxima",L"Derrape prolongado"},
  L"Hay una nueva versión disponible.",L"Instalada",L"Disponible",L"Cierra el juego antes de actualizar.\r\nNo se descarga ni instala automáticamente.",L"Cerrar y continuar",L"Abrir GitHub"},
 {L"es-MX",L"es-mx|mexican|spanish (mexico)|spanish mexican",L"Segoe UI",
  {L"Roce",L"Sentido contrario",L"Roce en sentido contrario",L"Salto",L"Tiempo en el aire",L"Derrape",L"Rebufo",L"Roces consecutivos",L"Velocidad máxima",L"Derrape prolongado"},
  L"Hay una nueva versión disponible.",L"Instalada",L"Disponible",L"Cierra el juego antes de actualizar.\r\nNo se descarga ni instala automáticamente.",L"Cerrar y continuar",L"Abrir GitHub"},
 {L"nl",L"nl|nl-nl|dutch",L"Segoe UI",
  {L"Bijna-botsing",L"Tegenliggersstrook",L"Tegenligger net ontweken",L"Sprong",L"Tijd in de lucht",L"Drift",L"Slipstream",L"Reeks bijna-botsingen",L"Topsnelheid",L"Lange drift"},
  L"Er is een nieuwe versie beschikbaar.",L"Geïnstalleerd",L"Beschikbaar",L"Sluit het spel voordat je bijwerkt.\r\nGeen automatische download of installatie.",L"Sluiten en doorgaan",L"GitHub openen"},
 {L"sv",L"sv|sv-se|swedish",L"Segoe UI",
  {L"Nära kollision",L"Mötande körfält",L"Nära möteskollision",L"Hopp",L"Tid i luften",L"Drift",L"Slipstream",L"Kollisionsundvikanden i rad",L"Toppfart",L"Lång drift"},
  L"En ny version finns tillgänglig.",L"Installerad",L"Tillgänglig",L"Stäng spelet innan du uppdaterar.\r\nIngen automatisk hämtning eller installation.",L"Stäng och fortsätt",L"Öppna GitHub"},
 {L"da",L"da|da-dk|danish",L"Segoe UI",
  {L"Næsten kollision",L"Modgående bane",L"Tæt på modkørende",L"Hop",L"Tid i luften",L"Drift",L"Slipstrøm",L"Nærved-kollisioner i træk",L"Topfart",L"Lang drift"},
  L"En ny version er tilgængelig.",L"Installeret",L"Tilgængelig",L"Luk spillet, før du opdaterer.\r\nIngen automatisk download eller installation.",L"Luk og fortsæt",L"Åbn GitHub"},
 {L"fi",L"fi|fi-fi|finnish",L"Segoe UI",
  {L"Läheltä piti",L"Vastaantulevien kaista",L"Vastaantulijan väistö",L"Hyppy",L"Ilmalento",L"Sivuluisu",L"Imu",L"Väistösarja",L"Huippunopeus",L"Pitkä sivuluisu"},
  L"Uusi versio on saatavilla.",L"Asennettu",L"Saatavilla",L"Sulje peli ennen päivittämistä.\r\nEi automaattista latausta tai asennusta.",L"Sulje ja jatka",L"Avaa GitHub"},
 {L"pl",L"pl|pl-pl|polish",L"Segoe UI",
  {L"Bliskie minięcie",L"Jazda pod prąd",L"Minięcie pod prąd",L"Skok",L"Czas w powietrzu",L"Drift",L"Jazda w cieniu",L"Seria bliskich minięć",L"Prędkość maksymalna",L"Długi drift"},
  L"Dostępna jest nowa wersja.",L"Zainstalowana",L"Dostępna",L"Zamknij grę przed aktualizacją.\r\nBez automatycznego pobierania i instalacji.",L"Zamknij i kontynuuj",L"Otwórz GitHub"},
 {L"ru",L"ru|ru-ru|russian",L"Segoe UI",
  {L"Опасное сближение",L"Встречная полоса",L"Сближение на встречной",L"Прыжок",L"Время в воздухе",L"Дрифт",L"Воздушный мешок",L"Серия сближений",L"Максимальная скорость",L"Затяжной дрифт"},
  L"Доступна новая версия.",L"Установлена",L"Доступна",L"Закройте игру перед обновлением.\r\nБез автоматической загрузки и установки.",L"Закрыть и продолжить",L"Открыть GitHub"},
 {L"ja",L"ja|jp|ja-jp|japanese",L"Yu Gothic UI",
  {L"ニアミス",L"対向車線",L"対向車ニアミス",L"ジャンプ",L"滑空",L"ドリフト",L"スリップストリーム",L"連続ニアミス",L"トップスピード",L"ドリフト継続"},
  L"新しいバージョンが公開されています。",L"現在",L"公開版",L"更新はゲーム終了後に行ってください。\r\n自動ダウンロード・インストールは行いません。",L"閉じて続行",L"GitHubを開く"},
 {L"ko",L"ko|ko-kr|korean",L"Malgun Gothic",
  {L"아슬아슬",L"역주행",L"역주행 아슬아슬",L"점프",L"체공",L"드리프트",L"슬립스트림",L"연속 아슬아슬",L"최고 속도",L"연속 드리프트"},
  L"새 버전이 출시되었습니다.",L"설치 버전",L"최신 버전",L"게임을 종료한 후 업데이트하세요.\r\n자동 다운로드 및 설치는 하지 않습니다.",L"닫고 계속",L"GitHub 열기"},
 {L"zh-TW",L"zh-tw|zh-hk|zh-hant|chinese|chinese (traditional)|traditional chinese",L"Microsoft JhengHei UI",
  {L"驚險閃避",L"逆向行駛",L"逆向驚險閃避",L"跳躍",L"滯空",L"甩尾",L"尾流加速",L"連續驚險閃避",L"極速行駛",L"持續甩尾"},
  L"新版本已發布。",L"目前版本",L"最新版本",L"請先關閉遊戲再更新。\r\n不會自動下載或安裝。",L"關閉並繼續",L"開啟 GitHub"},
 {L"zh-CN",L"zh-cn|zh-sg|zh-hans|chinese (simplified)|simplified chinese",L"Microsoft YaHei UI",
  {L"惊险闪避",L"逆向行驶",L"逆向惊险闪避",L"跳跃",L"滞空",L"漂移",L"尾流加速",L"连续惊险闪避",L"极速行驶",L"持续漂移"},
  L"新版本已发布。",L"当前版本",L"最新版本",L"请先关闭游戏再更新。\r\n不会自动下载或安装。",L"关闭并继续",L"打开 GitHub"},
 {L"th",L"th|th-th|thai",L"Leelawadee UI",
  {L"เฉียดชน",L"ขับสวนเลน",L"เฉียดรถสวนทาง",L"กระโดด",L"ลอยกลางอากาศ",L"ดริฟต์",L"สลิปสตรีม",L"เฉียดชนต่อเนื่อง",L"ความเร็วสูงสุด",L"ดริฟต์ต่อเนื่อง"},
  L"มีเวอร์ชันใหม่พร้อมใช้งาน",L"เวอร์ชันที่ติดตั้ง",L"เวอร์ชันใหม่",L"ปิดเกมก่อนอัปเดต\r\nไม่มีการดาวน์โหลดหรือติดตั้งอัตโนมัติ",L"ปิดและเล่นต่อ",L"เปิด GitHub"}
};
inline std::wstring normalizeLanguage(std::wstring_view value){
 value=value.substr(0,value.find(L';'));const auto a=value.find_first_not_of(L" \t\r\n");
 if(a==value.npos)return {};value=value.substr(a,value.find_last_not_of(L" \t\r\n")-a+1);
 const auto slash=value.find_last_of(L"\\/");if(slash!=value.npos)value.remove_prefix(slash+1);
 std::wstring s(value);for(auto& c:s){if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c==L'_')c=L'-';}
 if(s.size()>4&&s.substr(s.size()-4)==L".bin")s.resize(s.size()-4);return s;
}
inline const Locale* findLocale(std::wstring_view value){
 const auto s=normalizeLanguage(value);if(s.empty())return nullptr;
 for(const auto& locale:locales){std::wstring_view rest(locale.aliases);while(!rest.empty()){
  auto end=rest.find(L'|');if(rest.substr(0,end)==s)return &locale;if(end==rest.npos)break;rest.remove_prefix(end+1);
 }}return nullptr;
}
inline const Locale& resolveLocale(std::wstring_view explicitLanguage,std::wstring_view widescreen,
                                  std::wstring_view registry,std::wstring_view nativeFile,std::wstring_view savedSettings=L""){
 // A named override is authoritative. Unknown explicit names fall back to English.
 const auto setting=normalizeLanguage(explicitLanguage);
 if(!setting.empty()&&setting!=L"auto"){auto p=findLocale(setting);return p?*p:locales[0];}
 for(auto value:{widescreen,savedSettings,registry,nativeFile})if(auto p=findLocale(value))return *p;
 return locales[0];
}
inline std::wstring updateBody(const Locale& l,std::wstring_view current,std::wstring_view latest){
 return std::wstring(l.notice)+L"\r\n\r\n"+l.installed+L": "+std::wstring(current)+L"\r\n"+l.available+L": "+std::wstring(latest)+
  L"\r\n\r\n"+l.instructions+L"\r\n\r\ngithub.com/Zakkey250/MW-ActionNitro/releases";
}
}
