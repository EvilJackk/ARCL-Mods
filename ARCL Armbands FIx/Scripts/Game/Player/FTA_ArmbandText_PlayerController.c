//==============================================================================
// FTA_ArmbandText
//
// Dependent patch for FTA_EnforcedArmbands. The enforcement popups tell players
// to grab an armband from the "gloves section" of the arsenal, but the armbands
// are listed under Equipment. This rewrites the wording at display time instead
// of editing FTA_EnforcedArmbands_Config.conf, so FTA can keep shipping new
// stage messages without the fix having to be re-applied.
//
// Messages are built client-side in RpcDo_FTA_ShowPopup_O, so this runs on the
// client. Requires this addon to load after FTA_EnforcedArmbands (69B8B4C7FF4E64ED).
//==============================================================================

modded class SCR_PlayerController
{
	// Parallel find/replace lists, applied in order. Case sensitive - each casing
	// that appears in the stage messages needs its own entry.
	protected static const ref array<string> FTA_TEXTFIX_FIND = {
		"gloves section",
		"Gloves section",
		"GLOVES SECTION",
		"glove section",
		"Glove section",
		"GLOVE SECTION"
	};

	protected static const ref array<string> FTA_TEXTFIX_REPLACE = {
		"Equipment section",
		"Equipment section",
		"EQUIPMENT SECTION",
		"Equipment section",
		"Equipment section",
		"EQUIPMENT SECTION"
	};

	override protected void FTA_BuildMessageForStage(int stage, out string title, out string body)
	{
		super.FTA_BuildMessageForStage(stage, title, body);

		title = FTA_TextFix_Apply(title);
		body = FTA_TextFix_Apply(body);
	}

	protected string FTA_TextFix_Apply(string text)
	{
		if (text.IsEmpty())
			return text;

		int count = FTA_TEXTFIX_FIND.Count();
		if (FTA_TEXTFIX_REPLACE.Count() < count)
			count = FTA_TEXTFIX_REPLACE.Count();

		for (int i = 0; i < count; i++)
		{
			text.Replace(FTA_TEXTFIX_FIND[i], FTA_TEXTFIX_REPLACE[i]);
		}

		return text;
	}
}
