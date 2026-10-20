Safe Click Assist (Geode 5.11.2 / GD 2.2081 대상, 미검증 소스)

[방법 A] 내 PC에서 빌드
  geode sdk update
  geode sdk install-binaries
  (이 폴더에서) geode build
  -> build/ 안의 .geode 파일을 게임의 geode/mods/ 폴더에 넣기

[방법 B] GitHub에서 빌드 (SDK 설치 불필요)
  1. GitHub에서 새 저장소 만들기
  2. 이 폴더의 모든 파일(.github 폴더 포함)을 업로드
  3. Actions 탭 -> Build Geode Mod 실행
  4. 끝나면 Artifacts의 Build Output 다운로드 -> .geode 파일을 mods 폴더에 넣기
