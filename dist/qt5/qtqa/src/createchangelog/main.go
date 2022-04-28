package main

import (
	"bufio"
	"bytes"
	"fmt"
	"go/doc"
	"html/template"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"sort"
	"strings"

	"github.com/hashicorp/go-version"
)

type commandWithCapturedOutput struct {
	cmd    *exec.Cmd
	stdout bytes.Buffer
	stderr bytes.Buffer
}

func newCommandWithCapturedOutput(cmd *exec.Cmd) *commandWithCapturedOutput {
	result := &commandWithCapturedOutput{}
	result.cmd = cmd
	result.cmd.Stdout = &result.stdout
	result.cmd.Stderr = &result.stderr
	return result
}

func (c *commandWithCapturedOutput) run() (string, error) {
	err := c.cmd.Run()
	if err != nil {
		return "", fmt.Errorf("Error running %s: %s\nStdout: %s\nStderr: %s", strings.Join(c.cmd.Args, " "), err, c.stdout.String(), c.stderr.String())
	}
	return c.stdout.String(), err
}

func (c *commandWithCapturedOutput) runWithSpaceTrimmed() (string, error) {
	output, err := c.run()
	return strings.TrimSpace(output), err
}

func (c *commandWithCapturedOutput) runWithLines() ([]string, error) {
	output, err := c.run()
	if err != nil {
		return nil, err
	}
	var lines []string

	scanner := bufio.NewScanner(bytes.NewBufferString(output))
	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}

	if err := scanner.Err(); err != nil {
		return nil, fmt.Errorf("Error parsing line output: %s", err)
	}

	return lines, err
}

type repository string

func (repo repository) gitCommand(command string, parameters ...string) *commandWithCapturedOutput {
	parameters = append([]string{"--git-dir=" + string(repo), command}, parameters...)
	return newCommandWithCapturedOutput(exec.Command("git", parameters...))
}

func (repo repository) tags() ([]string, error) {
	return repo.gitCommand("tag", "-l").runWithLines()
}

func (repo repository) revList(revisionRange string) ([]string, error) {
	return repo.gitCommand("rev-list", revisionRange).runWithLines()
}

func (repo repository) catFile(objectType string, sha1 string) (string, error) {
	return repo.gitCommand("cat-file", objectType, sha1).run()
}

func detectCurrentQtVersionFromQMakeConf() (*version.Version, error) {
	qmakeConfFile, err := os.Open(".qmake.conf")
	if err != nil {
		defer qmakeConfFile.Close()
		return nil, fmt.Errorf("Error opening .qmake.conf - are you in the right directory?")
	}

	moduleVersionRegExp := regexp.MustCompile(`\s*MODULE_VERSION\s*=\s*(` + version.VersionRegexpRaw + `).*`)

	scanner := bufio.NewScanner(qmakeConfFile)
	for scanner.Scan() {
		line := scanner.Text()
		match := moduleVersionRegExp.FindStringSubmatch(line)
		if len(match) < 2 {
			continue
		}
		return version.NewVersion(match[1])
	}

	return nil, fmt.Errorf("Could not find MODULE_VERSION line in .qmake.conf to detect the current module version")
}

func determinePastQtVersions(repo repository) ([]*version.Version, error) {
	tags, err := repo.tags()
	if err != nil {
		return nil, fmt.Errorf("Error determining tags: %s", err)
	}

	var parsedVersions []*version.Version

	for _, tag := range tags {
		v, err := version.NewVersion(tag)
		if err != nil {
			continue
		}
		parsedVersions = append(parsedVersions, v)
	}

	return parsedVersions, nil
}

func filterOutPreReleases(versions []*version.Version) []*version.Version {
	var withoutPreRelease []*version.Version
	for _, v := range versions {
		if v.Prerelease() != "" {
			continue
		}
		withoutPreRelease = append(withoutPreRelease, v)
	}
	return withoutPreRelease
}

type sortedVersions []*version.Version

func (slice sortedVersions) Len() int {
	return len(slice)
}

func (slice sortedVersions) Less(i, j int) bool {
	return slice[i].LessThan(slice[j])
}

func (slice sortedVersions) Swap(i, j int) {
	slice[i], slice[j] = slice[j], slice[i]
}

type changeLogEntry struct {
	module string
	class  string
	text   string
}

func extractChangeLog(commitMessage string) (entry changeLogEntry) {
	scanner := bufio.NewScanner(bytes.NewBufferString(commitMessage))

	for scanner.Scan() {
		trimmedLine := strings.TrimSpace(scanner.Text())

		if entry.text == "" {
			withoutChangeLog := strings.TrimPrefix(trimmedLine, "[ChangeLog]")
			if withoutChangeLog == trimmedLine {
				continue
			}

			remainder := withoutChangeLog
			for {
				withoutBracket := strings.TrimPrefix(remainder, "[")
				if withoutBracket == remainder {
					break
				}
				var group string
				remainder = strings.TrimLeftFunc(withoutBracket, func(ch rune) bool {
					endOfGroup := ch == ']'
					if !endOfGroup {
						group += string(ch)
					}
					return !endOfGroup
				})
				remainder = strings.TrimPrefix(remainder, "]")
				if entry.module == "" {
					entry.module = group
				} else if entry.class == "" {
					entry.class = group
				} else {
					remainder = "[" + withoutBracket
					break
				}
			}

			entry.text = strings.TrimSpace(remainder)
		} else if trimmedLine == "" {
			break
		} else {
			entry.text = entry.text + " " + trimmedLine
		}
	}

	if entry.text != "" {
		for scanner.Scan() {
			trimmedLine := strings.TrimSpace(scanner.Text())
			if !strings.HasPrefix(strings.ToLower(trimmedLine), "task-number:") {
				continue
			}
			entry.text = "[" + strings.TrimSpace(trimmedLine[len("task-number:"):]) + "] " + entry.text
			break
		}
	}

	return
}

func determinePreviousMinorVersion(versions []*version.Version) *version.Version {
	lastVersion := len(versions) - 1
	previousMinor := lastVersion
	for previousMinor > 0 {
		if versions[previousMinor].Segments()[1] != versions[lastVersion].Segments()[1] {
			break
		}
		previousMinor--
	}
	return versions[previousMinor]
}

type templateVersion struct {
	Major int
	Minor int
	Patch int
}

func newTemplateVersion(v *version.Version) templateVersion {
	var result templateVersion
	result.Major = v.Segments()[0]
	result.Minor = v.Segments()[1]
	if len(v.Segments()) == 3 {
		result.Patch = v.Segments()[2]
	}
	return result
}

const minorVersionHeader = `Qt {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}} introduces many new features and improvements as well as bugfixes
over the {{.LastVersion.Major}}.{{.LastVersion.Minor}}.x series. For more details, refer to the online documentation
included in this distribution. The documentation is also available online:

  http://qt-project.org/doc/qt-{{.CurrentVersion.Major}}

The Qt version {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}} series is binary compatible with the {{.LastVersion.Major}}.{{.LastVersion.Minor}}.x series.
Applications compiled for {{.LastVersion.Major}}.{{.LastVersion.Minor}} will continue to run with {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}}.

Some of the changes listed in this file include issue tracking numbers
corresponding to tasks in the Qt Bug Tracker:

  http://bugreports.qt-project.org/

Each of these identifiers can be entered in the bug tracker to obtain more
information about a particular change.

****************************************************************************
*                   Important Behavior Changes                             *
****************************************************************************

****************************************************************************
*                          Library                                         *
****************************************************************************

`

const patchVersionHeader = `Qt {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}}.{{.CurrentVersion.Patch}} is a bug-fix release. It maintains both forward and backward
compatibility (source and binary) with Qt {{.LastVersion.Major}}.{{.LastVersion.Minor}}.{{.LastVersion.Patch}}.

For more details, refer to the online documentation included in this
distribution. The documentation is also available online:

  http://qt-project.org/doc/qt-{{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}}

The Qt version {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}} series is binary compatible with the {{.PreviousMinorVersion.Major}}.{{.PreviousMinorVersion.Minor}}.x series.
Applications compiled for {{.PreviousMinorVersion.Major}}.{{.PreviousMinorVersion.Minor}} will continue to run with {{.CurrentVersion.Major}}.{{.CurrentVersion.Minor}}.

Some of the changes listed in this file include issue tracking numbers
corresponding to tasks in the Qt Bug Tracker:

  http://bugreports.qt-project.org/

Each of these identifiers can be entered in the bug tracker to obtain more
information about a particular change.

****************************************************************************
*                   Important Behavior Changes                             *
****************************************************************************

****************************************************************************
*                          Library                                         *
****************************************************************************

`

func printParagraph(indent string, bulletCharacter byte, text string) {
	var buf bytes.Buffer
	doc.ToText(&buf, text, indent, indent, 79-len(indent))
	output := buf.Bytes()
	if len(output) > len(indent) && len(indent) > 2 {
		bulletPos := len(indent) - 2
		output[bulletPos] = bulletCharacter
	}
	os.Stdout.Write(output)
}

type classChange struct {
	className string
	changes   []string
}

type moduleChanges struct {
	moduleName              string
	classIndependentChanges []string
	changesPerClass         []*classChange
}

func (m *moduleChanges) append(entry changeLogEntry) {
	if entry.class == "" {
		m.classIndependentChanges = append(m.classIndependentChanges, entry.text)
		return
	}
	i := sort.Search(len(m.changesPerClass), func(i int) bool { return m.changesPerClass[i].className >= entry.class })
	if i < len(m.changesPerClass) && m.changesPerClass[i].className == entry.class {
		m.changesPerClass[i].changes = append(m.changesPerClass[i].changes, entry.text)
	} else {
		newClassChange := &classChange{
			className: entry.class,
			changes:   []string{entry.text},
		}
		m.changesPerClass = append(m.changesPerClass[:i], append([]*classChange{newClassChange}, m.changesPerClass[i:]...)...)
	}
}

func (m *moduleChanges) print() {
	fmt.Println(m.moduleName)
	fmt.Println(strings.Repeat("-", len(m.moduleName)))

	for _, change := range m.classIndependentChanges {
		fmt.Println()
		printParagraph("   ", '-', change)
	}

	for _, classChange := range m.changesPerClass {
		fmt.Println()
		fmt.Printf(" - %s:\n", classChange.className)
		for _, ch := range classChange.changes {
			printParagraph("     ", '*', ch)
		}
	}

	fmt.Println()
}

type topLevelChanges struct {
	globalChanges    []string
	changesPerModule []*moduleChanges
}

func (t *topLevelChanges) append(entry changeLogEntry) {
	if entry.module == "" {
		t.globalChanges = append(t.globalChanges, entry.text)
		return
	}
	i := sort.Search(len(t.changesPerModule), func(i int) bool { return t.changesPerModule[i].moduleName >= entry.module })
	if i < len(t.changesPerModule) && t.changesPerModule[i].moduleName == entry.module {
		t.changesPerModule[i].append(entry)
	} else {
		newModule := &moduleChanges{
			moduleName: entry.module,
		}
		newModule.append(entry)
		t.changesPerModule = append(t.changesPerModule[:i], append([]*moduleChanges{newModule}, t.changesPerModule[i:]...)...)
	}
}

func (t *topLevelChanges) print() {
	for _, change := range t.globalChanges {
		fmt.Println()
		printParagraph(" ", ' ', change)
		fmt.Println()
	}

	for _, module := range t.changesPerModule {
		module.print()
	}
}

func appMain() error {
	currentVersion, err := detectCurrentQtVersionFromQMakeConf()
	if err != nil {
		return err
	}

	workingDir, err := os.Getwd()
	if err != nil {
		return fmt.Errorf("Error determining current working directory: %s", err)
	}

	repo := repository(filepath.Join(workingDir, ".git"))
	pastVersions, err := determinePastQtVersions(repo)
	if err != nil {
		return err
	}

	pastVersions = filterOutPreReleases(pastVersions)

	sort.Sort(sortedVersions(pastVersions))

	lastVersion := pastVersions[len(pastVersions)-1]
	previousMinorVersion := determinePreviousMinorVersion(pastVersions)

	if !lastVersion.LessThan(currentVersion) {
		return fmt.Errorf("Detected current version %s is not newer than last tagged version %s - no changelog to create", currentVersion, lastVersion)
	}

	if len(lastVersion.Segments()) < 2 {
		return fmt.Errorf("Invalid last version: %s", lastVersion)
	}
	if len(currentVersion.Segments()) < 2 {
		return fmt.Errorf("Invalid current version: %s", currentVersion)
	}

	headerVariables := struct {
		LastVersion          templateVersion
		CurrentVersion       templateVersion
		PreviousMinorVersion templateVersion
	}{
		newTemplateVersion(lastVersion),
		newTemplateVersion(currentVersion),
		newTemplateVersion(previousMinorVersion),
	}

	var headerTemplate *template.Template

	// Same minor version -> then it's a patch release
	if currentVersion.Segments()[1] == lastVersion.Segments()[1] {
		headerTemplate = template.Must(template.New("patchVersionTemplate").Parse(patchVersionHeader))
	} else {
		headerTemplate = template.Must(template.New("minorVersionTemplate").Parse(minorVersionHeader))
	}

	if err := headerTemplate.Execute(os.Stdout, &headerVariables); err != nil {
		return err
	}

	commits, err := repo.revList("v" + lastVersion.String() + "..HEAD")
	if err != nil {
		return fmt.Errorf("Error determining commits for change log: %s", err)
	}

	var changes topLevelChanges

	for _, commitSha1 := range commits {
		commit, err := repo.catFile("commit", commitSha1)
		if err != nil {
			return fmt.Errorf("Error reading commit %s: %s", commitSha1, err)
		}
		entry := extractChangeLog(commit)
		if entry.text == "" {
			continue
		}
		//log.Println(commitSha1, entry.groups, entry.text)
		changes.append(entry)
	}

	changes.print()
	return nil
}

func main() {
	err := appMain()
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
